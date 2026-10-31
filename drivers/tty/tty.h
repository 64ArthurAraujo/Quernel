#pragma once
#include <stdint.h>
#include <drivers/tty.h>

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define RED_ON_WHITE 0xf4

/* Screen i/o ports */
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

/* cursor.c */
int get_offset(int col, int row);
int get_offset_row(int offset);
int get_offset_col(int offset);
int get_cursor_offset();
void set_cursor_offset(int offset);

/* utils.c */
void hexdump(const uint8_t *buf, int len);