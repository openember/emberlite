/*
 * UART echo server example using EmberLite HAL.
 *
 * Behavior:
 * - Read bytes from UART
 * - Split by line ending ('\n' or '\r')
 * - Echo the line back with a signature suffix
 */

#include "hal/hal.h"
#include "hal/hal_uart.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;

static void on_sig(int sig)
{
    (void)sig;
    g_stop = 1;
}

static void usage(const char* argv0)
{
    fprintf(stderr,
            "Usage: %s -d <device> [-b <baud>] [--suffix <text>]\n"
            "Example: %s -d /dev/ttyUSB0 -b 115200 --suffix \" [echo]\"\n",
            argv0,
            argv0);
}

static int parse_u32(const char* s, uint32_t* out)
{
    if (!s || !out) {
        return -1;
    }
    errno = 0;
    char* end = NULL;
    unsigned long v = strtoul(s, &end, 10);
    if (errno != 0 || !end || *end != '\0' || v > 0xFFFFFFFFu) {
        return -1;
    }
    *out = (uint32_t)v;
    return 0;
}

typedef struct {
    struct termios saved;
    int has_saved;
} term_guard_t;

static int stdin_set_raw(term_guard_t* g)
{
    if (!g) {
        return -1;
    }
    struct termios tio;
    if (tcgetattr(STDIN_FILENO, &tio) != 0) {
        return -1;
    }
    g->saved = tio;
    g->has_saved = 1;

    tio.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tio.c_oflag &= (tcflag_t)~OPOST;
    /* Keep ISIG so Ctrl-C still generates SIGINT. */
    tio.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | IEXTEN);
    tio.c_cflag &= (tcflag_t)~(CSIZE | PARENB);
    tio.c_cflag |= CS8;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &tio) != 0) {
        return -1;
    }
    return 0;
}

static void stdin_restore(term_guard_t* g)
{
    if (!g || !g->has_saved) {
        return;
    }
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &g->saved);
}

int main(int argc, char** argv)
{
    hal_uart_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.path = NULL;
    cfg.baudrate = 115200;
    cfg.data_bits = 8;
    cfg.stop_bits = 1;
    cfg.parity = 0;
    cfg.flow_control = 0;

    const char* suffix = " [echo]";

    for (int i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) {
            usage(argv[0]);
            return 0;
        } else if (!strcmp(a, "-d") && i + 1 < argc) {
            cfg.path = argv[++i];
        } else if (!strcmp(a, "-b") && i + 1 < argc) {
            uint32_t v;
            if (parse_u32(argv[++i], &v) != 0) {
                fprintf(stderr, "Invalid baudrate\n");
                return 2;
            }
            cfg.baudrate = v;
        } else if (!strcmp(a, "--suffix") && i + 1 < argc) {
            suffix = argv[++i];
        } else {
            fprintf(stderr, "Unknown arg: %s\n", a);
            usage(argv[0]);
            return 2;
        }
    }

    if (!cfg.path) {
        usage(argv[0]);
        return 2;
    }

    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    hal_uart_handle_t uart = NULL;
    hal_status_t st = hal_uart_open(&cfg, &uart);
    if (st != HAL_OK) {
        fprintf(stderr, "hal_uart_open(%s) failed: %s\n", cfg.path, hal_status_str(st));
        return 1;
    }

    term_guard_t tg;
    memset(&tg, 0, sizeof(tg));
    if (stdin_set_raw(&tg) != 0) {
        fprintf(stderr, "Failed to set stdin raw mode\n");
        (void)hal_uart_close(uart);
        return 1;
    }

    fprintf(stderr, "Echo server on %s @ %u. Suffix: \"%s\" (Ctrl-C to stop)\r\n", cfg.path, cfg.baudrate, suffix);

    uint8_t buf[256];
    char line[1024];
    size_t line_len = 0;
    size_t local_col = 0;

    const int uart_fd = hal_uart_get_fd(uart);
    if (uart_fd < 0) {
        fprintf(stderr, "hal_uart_get_fd failed\n");
        stdin_restore(&tg);
        (void)hal_uart_close(uart);
        return 1;
    }

    uint8_t tx[256];

    while (!g_stop) {
        struct pollfd pfds[2];
        memset(pfds, 0, sizeof(pfds));
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN;
        pfds[1].fd = uart_fd;
        pfds[1].events = POLLIN;

        int rc = poll(pfds, 2, 200);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "poll() failed\n");
            break;
        }

        if (pfds[0].revents & POLLIN) {
            ssize_t r = read(STDIN_FILENO, tx, sizeof(tx));
            if (r < 0) {
                if (errno == EINTR) {
                    continue;
                }
                fprintf(stderr, "\nstdin read error\n");
                break;
            }
            if (r == 0) {
                g_stop = 1;
                continue;
            }

            for (ssize_t i = 0; i < r; i++) {
                uint8_t c = tx[i];
                if (c == 0x7f || c == 0x08) { /* Backspace / DEL */
                    if (local_col > 0) {
                        const uint8_t bs_seq[3] = {'\b', ' ', '\b'};
                        (void)write(STDOUT_FILENO, bs_seq, sizeof(bs_seq));
                        local_col--;

                        uint8_t del = 0x7f;
                        size_t wn = 0;
                        st = hal_uart_write(uart, &del, 1, 1000, &wn);
                        if (st != HAL_OK) {
                            fprintf(stderr, "\nUART write error: %s\n", hal_status_str(st));
                            g_stop = 1;
                            break;
                        }
                    }
                    continue;
                }

                if (c == '\r' || c == '\n') {
                    const uint8_t crlf[2] = {'\r', '\n'};
                    size_t wn = 0;
                    st = hal_uart_write(uart, crlf, sizeof(crlf), 1000, &wn);
                    if (st != HAL_OK) {
                        fprintf(stderr, "\nUART write error: %s\n", hal_status_str(st));
                        g_stop = 1;
                        break;
                    }
                    (void)write(STDOUT_FILENO, crlf, sizeof(crlf));
                    local_col = 0;
                    continue;
                }

                size_t wn = 0;
                st = hal_uart_write(uart, &c, 1, 1000, &wn);
                if (st != HAL_OK) {
                    fprintf(stderr, "\nUART write error: %s\n", hal_status_str(st));
                    g_stop = 1;
                    break;
                }
                (void)write(STDOUT_FILENO, &c, 1);
                local_col++;
            }
        }

        size_t n = 0;
        if (pfds[1].revents & POLLIN) {
            st = hal_uart_read(uart, buf, sizeof(buf), 0, &n);
            if (st != HAL_OK) {
                fprintf(stderr, "UART read error: %s\n", hal_status_str(st));
                break;
            }
        }

        for (size_t i = 0; i < n; i++) {
            uint8_t c = buf[i];
            if (c == '\r' || c == '\n') {
                if (line_len == 0) {
                    continue;
                }

                const char crlf[2] = {'\r', '\n'};

                size_t wn = 0;
                st = hal_uart_write(uart, (const uint8_t*)line, line_len, 1000, &wn);
                if (st != HAL_OK) {
                    fprintf(stderr, "UART write error(line): %s\n", hal_status_str(st));
                    g_stop = 1;
                    break;
                }

                wn = 0;
                st = hal_uart_write(uart, (const uint8_t*)suffix, strlen(suffix), 1000, &wn);
                if (st != HAL_OK) {
                    fprintf(stderr, "UART write error(suffix): %s\n", hal_status_str(st));
                    g_stop = 1;
                    break;
                }

                wn = 0;
                st = hal_uart_write(uart, (const uint8_t*)crlf, sizeof(crlf), 1000, &wn);
                if (st != HAL_OK) {
                    fprintf(stderr, "UART write error(crlf): %s\n", hal_status_str(st));
                    g_stop = 1;
                    break;
                }

                line_len = 0;
                continue;
            }

            if (line_len + 1 < sizeof(line)) {
                line[line_len++] = (char)c;
            } else {
                /* line too long -> reset */
                line_len = 0;
            }
        }
    }

    stdin_restore(&tg);
    (void)hal_uart_close(uart);
    fprintf(stderr, "Bye.\n");
    return 0;
}

