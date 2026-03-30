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
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    fprintf(stderr, "Echo server on %s @ %u. Suffix: \"%s\" (Ctrl-C to stop)\n", cfg.path, cfg.baudrate, suffix);

    uint8_t buf[256];
    char line[1024];
    size_t line_len = 0;

    while (!g_stop) {
        size_t n = 0;
        st = hal_uart_read(uart, buf, sizeof(buf), 200, &n);
        if (st == HAL_ERR_TIMEOUT) {
            continue;
        }
        if (st != HAL_OK) {
            fprintf(stderr, "UART read error: %s\n", hal_status_str(st));
            break;
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

    (void)hal_uart_close(uart);
    fprintf(stderr, "Bye.\n");
    return 0;
}

