#pragma once

typedef struct 
{
    int hour;
    int minute;
    int second;
    int week_day;
    int month_day;
    int month;
    int year;
} rtc_date;

int unix_time_now();
rtc_date get_rtc_date();