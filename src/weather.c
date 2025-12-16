////////////////////////////////////////////////////////////////////////////////
// weather.c
////////////////////////////////////////////////////////////////////////////////

#include "merc.h"

#include "comm.h"
#include "weather.h"

#include "db.h"

#include "entities/descriptor.h"
#include "entities/room.h"

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

static void broadcast_daycycle_message(const char* message)
{
    Descriptor* d;

    if (message == NULL || message[0] == '\0')
        return;

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || d->character == NULL)
            continue;
        Mobile* ch = d->character;
        if (!IS_AWAKE(ch) || ch->in_room == NULL)
            continue;
        Room* room = ch->in_room;
        if (!IS_OUTSIDE(ch))
            continue;
        if (room->data && room->data->suppress_daycycle_messages)
            continue;
        send_to_char(message, ch);
    }
}

static void broadcast_weather_change_message(const char* message)
{
    Descriptor* d;

    if (message == NULL || message[0] == '\0')
        return;

    FOR_EACH(d, descriptor_list) {
        if (d->connected != CON_PLAYING || d->character == NULL)
            continue;
        Mobile* ch = d->character;
        if (!IS_AWAKE(ch) || ch->in_room == NULL)
            continue;
        if (!IS_OUTSIDE(ch))
            continue;
        send_to_char(message, ch);
    }
}

void update_weather_info()
{
    char daycycle_buf[MAX_STRING_LENGTH] = "";
    char weather_buf[MAX_STRING_LENGTH] = "";
    int diff;
    int old_hour = time_info.hour;

    switch (++time_info.hour) {
    case 5:
        weather_info.sunlight = SUN_LIGHT;
        strcat(daycycle_buf, "The day has begun.\n\r");
        break;

    case 6:
        weather_info.sunlight = SUN_RISE;
        strcat(daycycle_buf, "The sun rises in the east.\n\r");
        break;

    case 19:
        weather_info.sunlight = SUN_SET;
        strcat(daycycle_buf, "The sun slowly disappears in the west.\n\r");
        break;

    case 20:
        weather_info.sunlight = SUN_DARK;
        strcat(daycycle_buf, "The night has begun.\n\r");
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

    // Weather change.
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
            strcat(weather_buf, "The sky is getting cloudy.\n\r");
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_CLOUDY:
        if (weather_info.mmhg < 970
            || (weather_info.mmhg < 990 && number_bits(2) == 0)) {
            strcat(weather_buf, "It starts to rain.\n\r");
            weather_info.sky = SKY_RAINING;
        }

        if (weather_info.mmhg > 1030 && number_bits(2) == 0) {
            strcat(weather_buf, "The clouds disappear.\n\r");
            weather_info.sky = SKY_CLOUDLESS;
        }
        break;

    case SKY_RAINING:
        if (weather_info.mmhg < 970 && number_bits(2) == 0) {
            strcat(weather_buf, "Lightning flashes in the sky.\n\r");
            weather_info.sky = SKY_LIGHTNING;
        }

        if (weather_info.mmhg > 1030
            || (weather_info.mmhg > 1010 && number_bits(2) == 0)) {
            strcat(weather_buf, "The rain stopped.\n\r");
            weather_info.sky = SKY_CLOUDY;
        }
        break;

    case SKY_LIGHTNING:
        if (weather_info.mmhg > 1010
            || (weather_info.mmhg > 990 && number_bits(2) == 0)) {
            strcat(weather_buf, "The lightning has stopped.\n\r");
            weather_info.sky = SKY_RAINING;
            break;
        }
        break;
    }

    if (daycycle_buf[0] != '\0')
        broadcast_daycycle_message(daycycle_buf);

    if (weather_buf[0] != '\0')
        broadcast_weather_change_message(weather_buf);

    broadcast_room_period_messages(old_hour, time_info.hour);

    return;
}
