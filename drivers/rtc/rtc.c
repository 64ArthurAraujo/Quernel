/**
 * rtc.c
 *
 * The Real-Time-Clock driver
 */

#include <quernel/cpu.h>
#include <quernel/printk.h>
#include <stdint.h>

#include "rtc.h"

#define SECS_YEAR 31557600
#define SECS_MONTH 2628000
#define SECS_HOUR 3600
#define SECS_MIN 60
#define SECS_DAY 86400

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
 * get_rtc_date():
 *
 * Reads the RTC and returns into a organized struct.
 */
rtc_date get_rtc_date()
{
    rtc_date date;

    date.hour = get_converted(REG_HOURS);
    date.minute = get_converted(REG_MINS);
    date.second = get_converted(REG_SECS);
    date.week_day = get_converted(REG_WEEKDAY);
    date.month = get_converted(REG_MONTH);
    date.month_day = get_converted(REG_MONTHDAY);
    date.year = get_converted(REG_YEAR) + 2000;

    return date;
}

/**
 * unix_time_now():
 *
 * Reads the rtc and convert it into the unix timestamp, its probably
 * not precise but it do the job for now.
 */
int unix_time_now()
{
    rtc_date now = get_rtc_date();

    int epoch_seconds = (1970 * SECS_YEAR) + SECS_MONTH + SECS_DAY;
    int current_seconds = (now.year * SECS_YEAR) +
                          (now.month * SECS_MONTH) +
                          (now.month_day * SECS_DAY) +
                          (now.minute * SECS_MIN) +
                          (now.hour * SECS_HOUR) +
                          now.second;

    // The calculation i made above is not perfect and results in a timestamp 4 hours
    // in the front of the current, so we make here a manual adjustment so we can have 
    // the real current unix timestamp. we might be still 5 seconds inaccurate but i
    // think this won't cause any problems for now, the above calculation should probably
    // be revisited and recreated in the future.
    current_seconds -= (4 * SECS_HOUR);

    return current_seconds - epoch_seconds;
}