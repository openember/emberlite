/*
 * Minimal picocom-like UART terminal example using EmberLite HAL.
 *
 * Keys:
 *   Ctrl-A, Ctrl-X : exit
 */

#include "hal/hal_uart.h"
#include "hal/hal.h"

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

static void on_sigint(int sig)
{
    (void)sig;
    g_stop = 1;
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
    tio.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
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

static void usage(const char* argv0)
{
    fprintf(stderr,
            "Usage: %s -d <device> [-b <baud>] [--databits N] [--stopbits N] [--parity none|odd|even] [--flow none|hw|sw]\\n"
            "Example: %s -d /dev/ttyUSB0 -b 115200\\n"
            "Exit: Ctrl-A then Ctrl-X\\n",
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
    if (errno != 0 || !end || *end != '\0') {
        return -1;
    }
    if (v > 0xFFFFFFFFu) {
        return -1;
    }
    *out = (uint32_t)v;
    return 0;
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
                fprintf(stderr, "Invalid baudrate\\n");
                return 2;
            }
            cfg.baudrate = v;
        } else if (!strcmp(a, "--databits") && i + 1 < argc) {
            uint32_t v;
            if (parse_u32(argv[++i], &v) != 0 || (v < 5 || v > 8)) {
                fprintf(stderr, "Invalid databits (5..8)\\n");
                return 2;
            }
            cfg.data_bits = (uint8_t)v;
        } else if (!strcmp(a, "--stopbits") && i + 1 < argc) {
            uint32_t v;
            if (parse_u32(argv[++i], &v) != 0 || (v != 1 && v != 2)) {
                fprintf(stderr, "Invalid stopbits (1|2)\\n");
                return 2;
            }
            cfg.stop_bits = (uint8_t)v;
        } else if (!strcmp(a, "--parity") && i + 1 < argc) {
            const char* p = argv[++i];
            if (!strcmp(p, "none")) {
                cfg.parity = 0;
            } else if (!strcmp(p, "odd")) {
                cfg.parity = 1;
            } else if (!strcmp(p, "even")) {
                cfg.parity = 2;
            } else {
                fprintf(stderr, "Invalid parity (none|odd|even)\\n");
                return 2;
            }
        } else if (!strcmp(a, "--flow") && i + 1 < argc) {
            const char* f = argv[++i];
            if (!strcmp(f, "none")) {
                cfg.flow_control = 0;
            } else if (!strcmp(f, "hw")) {
                cfg.flow_control = 1;
            } else if (!strcmp(f, "sw")) {
                cfg.flow_control = 2;
            } else {
                fprintf(stderr, "Invalid flow (none|hw|sw)\\n");
                return 2;
            }
        } else {
            fprintf(stderr, "Unknown arg: %s\\n", a);
            usage(argv[0]);
            return 2;
        }
    }

    if (!cfg.path) {
        usage(argv[0]);
        return 2;
    }

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    hal_uart_handle_t uart = NULL;
    hal_status_t st = hal_uart_open(&cfg, &uart);
    if (st != HAL_OK) {
        fprintf(stderr, "hal_uart_open(%s) failed: %s\\n", cfg.path, hal_status_str(st));
        return 1;
    }

    term_guard_t tg;
    memset(&tg, 0, sizeof(tg));
    if (stdin_set_raw(&tg) != 0) {
        fprintf(stderr, "Failed to set stdin raw mode\\n");
        (void)hal_uart_close(uart);
        return 1;
    }

    fprintf(stderr, "Connected to %s @ %u. Exit with Ctrl-A then Ctrl-X.\\n", cfg.path, cfg.baudrate);

    const int uart_fd = hal_uart_get_fd(uart);
    if (uart_fd < 0) {
        fprintf(stderr, "hal_uart_get_fd failed\\n");
        stdin_restore(&tg);
        (void)hal_uart_close(uart);
        return 1;
    }

    uint8_t rx[1024];
    uint8_t tx[256];
    int saw_ctrl_a = 0;

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
            fprintf(stderr, "poll() failed\\n");
            break;
        }

        if (pfds[1].revents & POLLIN) {
            size_t n = 0;
            st = hal_uart_read(uart, rx, sizeof(rx), 0, &n);
            if (st != HAL_OK) {
                fprintf(stderr, "\\nUART read error: %s\\n", hal_status_str(st));
                break;
            }
            if (n > 0) {
                (void)write(STDOUT_FILENO, rx, n);
            }
        }

        if (pfds[0].revents & POLLIN) {
            ssize_t r = read(STDIN_FILENO, tx, sizeof(tx));
            if (r < 0) {
                if (errno == EINTR) {
                    continue;
                }
                fprintf(stderr, "\\nstdin read error\\n");
                break;
            }
            if (r == 0) {
                g_stop = 1;
                continue;
            }

            for (ssize_t i = 0; i < r; i++) {
                uint8_t c = tx[i];
                if (saw_ctrl_a) {
                    saw_ctrl_a = 0;
                    if (c == 0x18) { /* Ctrl-X */
                        g_stop = 1;
                        break;
                    }
                    /* If not Ctrl-X, send both bytes: Ctrl-A then c */
                    uint8_t seq[2] = {0x01, c};
                    size_t wn = 0;
                    st = hal_uart_write(uart, seq, sizeof(seq), 1000, &wn);
                    if (st != HAL_OK) {
                        fprintf(stderr, "\\nUART write error: %s\\n", hal_status_str(st));
                        g_stop = 1;
                        break;
                    }
                    continue;
                }

                if (c == 0x01) { /* Ctrl-A */
                    saw_ctrl_a = 1;
                    continue;
                }

                size_t wn = 0;
                st = hal_uart_write(uart, &c, 1, 1000, &wn);
                if (st != HAL_OK) {
                    fprintf(stderr, "\\nUART write error: %s\\n", hal_status_str(st));
                    g_stop = 1;
                    break;
                }
            }
        }
    }

    stdin_restore(&tg);
    (void)hal_uart_close(uart);
    fprintf(stderr, "\\nBye.\\n");
    return 0;
}

