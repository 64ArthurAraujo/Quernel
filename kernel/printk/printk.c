#include "../../libc/memory.h"
#include "../../drivers/tty.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

enum format_state
{
    FORMAT_STATE_NONE, /* Just a string part */
    FORMAT_STATE_CHAR,
    FORMAT_STATE_STR,
    FORMAT_STATE_PERCENT_CHAR,
    FORMAT_STATE_INVALID,
};

struct fmt
{
    const char *str;
    uint8_t state; // enum format_state
    uint8_t size;  // size of numbers
};

struct fmt format_decode(struct fmt fmt)
{
    const char *start = fmt.str;

    /* By default */
    fmt.state = FORMAT_STATE_NONE;

    for (; *fmt.str; fmt.str++)
    {
        if (*fmt.str == '%')
            break;
    }

    /* Return the current non-format string */
    if (fmt.str != start || !*fmt.str)
        return fmt;

    // skip the '%'
    *++fmt.str;

    static const struct format_state
    {
        uint8_t state;
        uint8_t size;
        uint8_t flag_or_double_size;
        uint8_t base;
    } lookup_state[256] = {
        // Non-numeric formats
        ['c'] = {FORMAT_STATE_CHAR},
        ['s'] = {FORMAT_STATE_STR},
        ['%'] = {FORMAT_STATE_PERCENT_CHAR},
    };

    const struct format_state *p = lookup_state + (uint8_t)*fmt.str;

    if (p->state)
    {
        fmt.state = p->state;
        fmt.str++;
        return fmt;
    }

    fmt.state = FORMAT_STATE_INVALID;
    return fmt;
}

char *string(char *buf, char *end, const char *s)
{
    if (!s)
        s = "(null)";
    
    while (*s)
    {
        if (buf < end)
            *buf = *s;
        ++buf;
        ++s;
    }

    return buf;
}

/**
 * vsnprintf - Format a string and place it in the buffer
 * @buf: The buffer to place the result into
 * @fmt_str: The format string to use
 * @args: Arguments for the format string
 *
 * vsnprintf is where the real parsing of the args happens
 *
 */
int vsnprintf(char *buf, size_t size, const char *fmt_str, va_list args)
{
    char *str, *end;
    size_t ret_size;
    struct fmt fmt = {
        .str = fmt_str,
        .state = FORMAT_STATE_NONE,
    };

    str = buf;
    end = buf + size;

    // Make sure end is always >= buf
    if (end < buf)
    {
        end = ((void *)-1);
        size = end - buf;
    }

    while (*fmt.str)
    {
        const char *old_fmt = fmt.str;

        fmt = format_decode(fmt);

        switch (fmt.state)
        {
        case FORMAT_STATE_NONE:
        {
            int read = fmt.str - old_fmt;
            if (str < end)
            {
                int copy = read;
                if (copy > end - str)
                    copy = end - str;
                memcpy(str, old_fmt, copy);
            }
            str += read;
            continue;
        }

        case FORMAT_STATE_CHAR:
        {
            char c;

            c = (uint8_t)va_arg(args, int);

            if (str < end)
                *str = c;
            ++str;

            continue;
        }

        case FORMAT_STATE_STR:
        {
            str = string(str, end, va_arg(args, char *));
            continue;
        }

        case FORMAT_STATE_PERCENT_CHAR:
        {
            if (str < end)
                *str = '%';
            ++str;
            continue;
        }

        default:
            goto out;
        }
    }

out:
    if (size > 0)
    {
        if (str < end)
            *str = '\0';
        else
            end[-1] = '\0';
    }

    /* the trailing null byte doesn't count towards the total */
    ret_size = str - buf;

    // 0x7fffffff = INT_MAX
    if (ret_size > 0x7fffffff)
        ret_size = 0x7fffffff;
    return ret_size;
}

uint16_t printk_sprint(char *text, size_t size, const char *fmt, va_list args)
{
    uint16_t text_len;

    text_len = vsnprintf(text, size, fmt, args);

    if (text_len && text[text_len - 1] == '\n')
    {
        text_len--;
    }

    return text_len;
}

int vprintk(const char *fmt, va_list args)
{
    char *text_buf;
    uint16_t text_len;
    va_list args2;
    uint16_t reserve_size;
    char prefix_buf[8];

    va_copy(args2, args);
    reserve_size = vsnprintf(&prefix_buf[0], sizeof(prefix_buf), fmt, args2) + 1;
    va_end(args2);

    /* fill message */
    text_len = printk_sprint(&text_buf[0], reserve_size, fmt, args);

    kprint(text_buf);
}

int printk(const char *fmt, ...)
{
    va_list args;
    int r;

    va_start(args, fmt);
    r = vprintk(fmt, args);
    va_end(args);

    return r;
}