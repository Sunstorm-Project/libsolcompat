#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <solcompat/clock.h>

int main(void)
{
    struct tm t = {0};
    if (putenv("TZ=EST5EDT"))
        return 1;
    tzset();
    t.tm_year = 100;
    t.tm_mon = 1;
    t.tm_mday = 29;
    t.tm_isdst = 1;
    if (timegm(&t) != (time_t)951782400 || t.tm_yday != 59 || t.tm_wday != 2 || t.tm_isdst)
        return 2;
    memset(&t, 0, sizeof(t));
    t.tm_year = 71;
    t.tm_mon = -12;
    t.tm_mday = 1;
    t.tm_sec = -1;
    errno = 0;
    if (timegm(&t) != (time_t)-1 || errno == EOVERFLOW || t.tm_year != 69 ||
        t.tm_mon != 11 || t.tm_mday != 31 || t.tm_hour != 23 || t.tm_sec != 59)
        return 3;
    if (sizeof(time_t) == 4) {
        t.tm_year = INT_MAX;
        if (timegm(&t) != (time_t)-1 || errno != EOVERFLOW)
            return 4;
    }
    return strcmp(getenv("TZ"), "EST5EDT") != 0;
}
