#pragma once
#include <stdint.h>

void tty_clear();
void tty_emit(char *message);
void tty_emit_at(char *message, int col, int row);
void hexdump(const uint8_t *buf, int len);