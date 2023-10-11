////////////////////////////////////////////////////////////////////////////////
// weather.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "weather.h"

#include "db.h"

#include "entities/descriptor.h"

TimeInfo time_info;
WeatherInfo weather_info;

void init_weather_info()
{
    long lhour = (long)((current_time - 1684281600)
        / (PULSE_TICK / PULSE_PER_SECOND));
    long lday = lhour / 24;
    long lmonth = lday / 35;

    time_info.hour = lhour % 24;
    time_info.day = lday % 35;
    time_info.month = lmonth % 17;
    time_info.year = lmonth / 17;

    if (time_info.hour < 5)
        weather_info.sunlight = SUN_DARK;
    else if (time_info.hour < 6)
        weather_info.sunlight = SUN_RISE;
    else if (time_info.hour < 19)
        weather_info.sunlight = SUN_LIGHT;
    else if (time_info.hour < 20)
        weather_info.sunlight = SUN_SET;
    else
        weather_info.sunlight = SUN_DARK;

    weather_info.change = 0;
    weather_info.mmhg = 960;
    if (time_info.month >= 7 && time_info.month <= 12)
        weather_info.mmhg += number_range(1, 50);
    else
        weather_info.mmhg += number_range(1, 80);

    if (weather_info.mmhg <= 980)
        weather_info.sky = SKY_LIGHTNING;
    else if (weather_info.mmhg <= 1000)
        weather_info.sky = SKY_RAINING;
    else if (weather_info.mmhg <= 1020)
        weather_info.sky = SKY_CLOUDY;
    else
        weather_info.sky = SKY_CLOUDLESS;
}

void update_weather_info()
{
    char buf[MAX_STRING_LENGTH] = "";
    Descriptor* d;
    int diff;

    buf[0] = '\0';

    switch (++time_info.hour) {
    case 5:
        weather_info.sunlight = SUN_LIGHT;
        strcat(buf, "The day has begun.\n\r");
        break;

    case 6:
        weather_info.sunlight = SUN_RISE;
        strcat(buf, "The sun rises in the east.\n\r");
        break;

    case 19:
        weather_info.sunlight = SUN_SET;
        strcat(buf, "The sun slowly disappears in the west.\n\r");
        break;

    case 20:
        weather_info.sunlight = SUN_DARK;
        strcat(buf, "The night has begun.\n\r");
        break;

    case 24:
        time_info.hour = 0;
        time_info.day++;
        break;
    }

    if (time_info.day >= 35) {
        time_info.day = 0;
        time_info.month++;
    }

    if (time_info.month >= 17) {
        time_info.month = 0;
        time_info.year++;
    }

    /*
     * Weather change.
     */
    if (time_info.month >= 9 && time_info.month <= 16)
        diff = weather_info.mmhg > 985 ? -2 : 2;
    else
        diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change = UMAX(weather_info.change, -12);
    weather_info.change = UMIN(weather_info.change, 12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg = UMAX(weather_info.mmhg, 960);
    weather_info.mmhg = UMIN(weather_info.mmhg, 1040);

    switch (weather_info.sky) {
    default:
        bug("Weather_update: bad sky %d.", weather_info.sky);
        weather_info.sky = SKY_CLOUDLESS;
        break;

    case SKY_CLOUDLESS:
        if (weather_info.mmhg < 990
            || (weather_info.mmhg < 1010 && number_bits(2) == 0)) {
            strcat(buf, "The sky is getting cloudy.\n\r");
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_CLOUDY:
        if (weather_info.mmhg < 970
            || (weather_info.mmhg < 990 && number_bits(2) == 0)) {
            strcat(buf, "It starts to rain.\n\r");
            weather_info.sky = SKY_RAINING;
        }

        if (weather_info.mmhg > 1030 && number_bits(2) == 0) {
            strcat(buf, "The clouds disappear.\n\r");
            weather_info.sky = SKY_CLOUDLESS;
        }
        break;

    case SKY_RAINING:
        if (weather_info.mmhg < 970 && number_bits(2) == 0) {
            strcat(buf, "Lightning flashes in the sky.\n\r");
            weather_info.sky = SKY_LIGHTNING;
        }

        if (weather_info.mmhg > 1030
            || (weather_info.mmhg > 1010 && number_bits(2) == 0)) {
            strcat(buf, "The rain stopped.\n\r");
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_LIGHTNING:
        if (weather_info.mmhg > 1010
            || (weather_info.mmhg > 990 && number_bits(2) == 0)) {
            strcat(buf, "The lightning has stopped.\n\r");
            weather_info.sky = SKY_RAINING;
            break;
        }
        break;
    }

    if (buf[0] != '\0') {
        FOR_EACH(d, descriptor_list) {
            if (d->connected == CON_PLAYING && IS_OUTSIDE(d->character)
                && IS_AWAKE(d->character))
                send_to_char(buf, d->character);
        }
    }

    return;
}
