#include <quernel/cpu.h>
#include <memory.h>

#include "tty.h"

int emit_char(char c, int col, int row, char attr)
{
    unsigned char *vidmem = (unsigned char *)VIDEO_ADDRESS;
    if (!attr)
        attr = WHITE_ON_BLACK;

    // Error control: print a red 'E' if the coords aren't right
    if (col >= MAX_COLS || row >= MAX_ROWS)
    {
        vidmem[2 * (MAX_COLS) * (MAX_ROWS)-2] = '>';
        vidmem[2 * (MAX_COLS) * (MAX_ROWS)-1] = RED_ON_WHITE;
        return get_offset(col, row);
    }

    int offset;
    if (col >= 0 && row >= 0)
        offset = get_offset(col, row);
    else
        offset = get_cursor_offset();

    if (c == '\n')
    {
        row = get_offset_row(offset);
        offset = get_offset(0, row + 1);
    }
    else
    {
        vidmem[offset] = c;
        vidmem[offset + 1] = attr;
        offset += 2;
    }

    // scroll implementation
    if (offset >= MAX_ROWS * MAX_COLS * 2)
    {
        int i;

        for (i = 1; i < MAX_ROWS; i++)
        {
            memcpy(
                (char *)get_offset(0, i - 1) + VIDEO_ADDRESS,
                (char *)get_offset(0, i) + VIDEO_ADDRESS,
                MAX_COLS * 2);
        }

        char *last_line = (char *)(get_offset(0, MAX_ROWS - 1) + VIDEO_ADDRESS);

        for (i = 0; i < MAX_COLS * 2; i++)
            last_line[i] = 0;

        offset -= 2 * MAX_COLS;
    }

    set_cursor_offset(offset);
    return offset;
}

void tty_emit(char *message)
{
    tty_emit_at(message, -1, -1);
}

/**
 * Show a message on the specified location
 * If col, row, are negative, we will use the current offset
 */
void tty_emit_at(char *message, int col, int row)
{
    /* Set cursor if col/row are negative */
    int offset;
    if (col >= 0 && row >= 0)
        offset = get_offset(col, row);
    else
    {
        offset = get_cursor_offset();
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }

    // Loop through message and print it
    int i = 0;
    while (message[i] != 0)
    {
        offset = emit_char(message[i++], col, row, WHITE_ON_BLACK);
        // Compute row/col for next iteration
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }
}

void tty_clear()
{
    int screen_size = MAX_COLS * MAX_ROWS;
    int i;
    char *screen = (char *)VIDEO_ADDRESS;

    for (i = 0; i < screen_size; i++)
    {
        screen[i * 2] = ' ';
        screen[i * 2 + 1] = WHITE_ON_BLACK;
    }
    set_cursor_offset(get_offset(0, 0));
}