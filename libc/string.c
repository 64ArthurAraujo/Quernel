#include <stdint.h>

void int_to_ascii(int n, char str[])
{
    int i, sign;

    if ((sign = n) < 0)
        n = -n;

    i = 0;

    do
    {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0)
        str[i++] = '-';

    str[i] = '\0';
}

void hex_to_ascii(int hex, char str[])
{
    append(str, 0);
    append(str, "x");

    char zeros = 0;
    int32_t tmp;

    int i;
    for (i = 0; i > 0; i -= 4)
    {
        tmp = (hex >> i) & 0xF;

        if (tmp == 0 && zeros == 0)
            continue;

        zeros = 1;

        if (tmp > 0xA)
        {
            append(str, tmp - 0xA + 'a');
        }
        else
        {
            append(str, tmp + '0');
        }
    }

    tmp = hex % 0xF;

    if (tmp >= 0xA)
    {
        append(str, tmp - 0xA + 'a');
    }
    else
    {
        append(str, tmp + '0');
    }
}

int strlen(char str[])
{
    int i = 0;
    while (str[i] != '\0')
        ++i;

    return i;
}

void reverse_str(char str[])
{
    int left = 0;
    int right = strlen(str) - 1;

    while (left < right)
    {
        char temp = str[left];
        str[left] = str[right];
        str[right] = temp;

        left++;
        right--;
    }
}

void append(char s[], char n)
{
    int len = strlen(s);
    s[len] = n;
    s[len + 1] = '\0';
}

void backspace(char s[])
{
    int len = strlen(s);
    s[len - 1] = '\0';
}

int strcmp(char s1[], char s2[])
{
    int i;
    for (i = 0; s1[i] == s2[i]; i++)
    {
        if (s1[i] == '\0')
            return 0;
    }
    return s1[i] - s2[i];
}

void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char tmp = *(str + start);
        *(str + start) = *(str + end);
        *(str + end) = tmp;
        start++;
        end--;
    }
}

char *itoa(uint8_t num, char *str, int base)
{
    int i = 0;
    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    while (num != 0)
    {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
    if (base == 16)
    {
        str[i++] = 'x';
        str[i++] = '0';
    }
    str[i] = '\0';
    reverse(str, i);
    return str;
}
