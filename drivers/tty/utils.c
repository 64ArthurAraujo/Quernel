#include "tty.h"

static void delay(volatile uint32_t count)
{
    while (count--)
        ;
}

static const char hexdigits[] = "0123456789abcdef";

void hexdump(const uint8_t *buf, int len)
{
    char line[80];

    for (int i = 0; i < len; i += 16)
    {
        int p = 0;

        /* offset: 4 hex digits */
        line[p++] = hexdigits[(i >> 12) & 0xF];
        line[p++] = hexdigits[(i >> 8) & 0xF];
        line[p++] = hexdigits[(i >> 4) & 0xF];
        line[p++] = hexdigits[i & 0xF];
        line[p++] = ':';
        line[p++] = ' ';

        /* 16 bytes as hex */
        for (int j = 0; j < 16; j++)
        {
            uint8_t b = buf[i + j];
            line[p++] = hexdigits[b >> 4];
            line[p++] = hexdigits[b & 0xF];
            line[p++] = ' ';
        }

        /* ASCII column */
        line[p++] = '|';
        for (int j = 0; j < 16; j++)
        {
            uint8_t c = buf[i + j];
            line[p++] = (c >= 32 && c < 127) ? c : '.';
        }
        line[p++] = '|';
        line[p++] = '\n';
        line[p] = '\0';

        tty_emit_at(line, -1, -1);
        delay(20000000);
    }
}
