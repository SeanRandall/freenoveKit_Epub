#include <errno.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dtmmedefs.h"

/* The reader always opens DECtalk's in-memory output path.  These two
 * compatibility functions satisfy dormant Unix audio/thread references
 * without pulling the OSS device layer into the ESP32 firmware. */
LONG OSS_WaveInit(void)
{
    return MMSYSERR_NOTSUPPORTED;
}

int nanosleep(const struct timespec *requested, struct timespec *remaining)
{
    if (requested == NULL || requested->tv_sec < 0 ||
            requested->tv_nsec < 0 || requested->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }

    uint64_t usec = (uint64_t)requested->tv_sec * 1000000ULL;
    usec += ((uint64_t)requested->tv_nsec + 999ULL) / 1000ULL;
    TickType_t ticks = pdMS_TO_TICKS((usec + 999ULL) / 1000ULL);
    if (ticks == 0 && usec != 0) {
        ticks = 1;
    }
    if (ticks != 0) {
        vTaskDelay(ticks);
    }
    if (remaining != NULL) {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
    }
    return 0;
}
