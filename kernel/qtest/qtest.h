#pragma once

static int failed;

static inline void expect(int cond, const char *msg)
{
    if (cond)
    {
        pr_info("ok:   %s\n", msg);
    }
    else
    {
        pr_err("FAIL: %s\n", msg);
    }
}

static inline void make_name(char *out, uint32_t i)
{
    char tmp[12];
    int n = 0;
    do
    {
        tmp[n++] = '0' + (i % 10);
        i /= 10;
    } while (i);
    out[0] = 'd';
    for (int k = 0; k < n; k++)
        out[1 + k] = tmp[n - 1 - k];
    out[1 + n] = 0;
}
