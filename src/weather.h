////////////////////////////////////////////////////////////////////////////////
// weather.h
////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef MUD98__WEATHER_H
#define MUD98__WEATHER_H

typedef enum sun_t {
    SUN_DARK    = 0,
    SUN_RISE    = 1,
    SUN_LIGHT   = 2,
    SUN_SET     = 3,
} Sun;

typedef enum sky_t {
    SKY_CLOUDLESS   = 0,
    SKY_CLOUDY      = 1,
    SKY_RAINING     = 2,
    SKY_LIGHTNING   = 3,
} Sky;

typedef struct time_info_t {
    int hour;
    int day;
    int month;
    int year;
} TimeInfo;

typedef struct weather_info_t {
    int mmhg;
    int change;
    Sky sky;
    Sun sunlight;
} WeatherInfo;

void init_weather_info(void);
void update_weather_info(void);

extern TimeInfo time_info;
extern WeatherInfo weather_info;

#endif // !MUD98__WEATHER_H
