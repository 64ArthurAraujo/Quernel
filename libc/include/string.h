#pragma once

#include <stdint.h>

void int_to_ascii(int n, char str[]);
void hex_to_ascii(int hex, char str[]);
void reverse_str(char str[]);
int strlen(char str[]);
void append(char s[], char n);
void backspace(char s[]);
int strcmp(char s1[], char s2[]);
char *itoa(uint8_t num, char *str, int base);