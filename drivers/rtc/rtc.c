/**
 * rtc.c
 *
 * The Real-Time-Clock driver
 */

#include <quernel/cpu.h>
#include <quernel/printk.h>
#include <stdint.h>

#include "rtc.h"

#define SECS_IN_A_YEAR 31557600
#define SECS_IN_A_MONTH 2628000
#define SECS_IN_A_HOUR 3600
#define SECS_IN_A_MIN 60



static uint8_t get_registry(int r)
{
    port_byte_out(CMOS_ADDR, r);
    return port_byte_in(CMOS_DATA);
}

static int get_converted(int reg)
{
    uint8_t raw = get_registry(reg);
    return ((raw / 16) * 10) + (raw & 0xf);
}

/**
 * unix_time_now():
 *
 * Reads the rtc and convert it into the unix timestamp, its probably
 * not precise but it do the job for now.
 */
int unix_time_now()
{
    int year = get_converted(REG_YEAR) + 2000; // gambiarra that can last 80 years (very efficient)
    int e_year = 1970;

    int month_day = get_converted(REG_MONTHDAY);
    int e_month_day = 1;

    int month = get_converted(REG_MONTH);
    int e_month = 1;

    // idk if its counting 12 hours or 24, probably if its 12 when we write a file at 9 am and 9 pm they will have the same timestamp
    int hours = get_converted(REG_HOURS); 
    int minutes = get_converted(REG_MINS);
    int seconds = get_converted(REG_SECS);

    int epoch_seconds = (e_year * SECS_IN_A_YEAR) + (e_month * SECS_IN_A_MONTH);
    int current_seconds = (year * SECS_IN_A_YEAR) + (month * SECS_IN_A_MONTH) + (hours * SECS_IN_A_HOUR) + (minutes * SECS_IN_A_MIN) + seconds;

    return current_seconds - epoch_seconds;
}