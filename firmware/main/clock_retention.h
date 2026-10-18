#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

/* STORE0 and STORE1 are used for reader/network-partition handoff.  STORE2
   and STORE3 retain a wall-clock snapshot across resets without wearing NVS.
   They are cleared by a genuine loss of power, which is appropriate because
   this board has no independent real-time-clock battery. */
#define RR_CLOCK_RETAIN_MAGIC UINT32_C(0x52525443) /* "RRTC" */
#define RR_CLOCK_EARLIEST     UINT32_C(1704067200) /* 2024-01-01 UTC */
#define RR_CLOCK_LATEST       UINT32_C(4102444800) /* 2100-01-01 UTC */

static inline bool rr_clock_time_valid(time_t value)
{
    return value >= (time_t)RR_CLOCK_EARLIEST
        && (uint64_t)value < (uint64_t)RR_CLOCK_LATEST;
}

static inline void rr_clock_retain_now(void)
{
    time_t now = time(NULL);
    if (!rr_clock_time_valid(now))
        return;
    REG_WRITE(RTC_CNTL_STORE2_REG, (uint32_t)now);
    REG_WRITE(RTC_CNTL_STORE3_REG, RR_CLOCK_RETAIN_MAGIC);
}

static inline bool rr_clock_restore(void)
{
    if (REG_READ(RTC_CNTL_STORE3_REG) != RR_CLOCK_RETAIN_MAGIC)
        return false;
    uint32_t saved = REG_READ(RTC_CNTL_STORE2_REG);
    if (!rr_clock_time_valid((time_t)saved))
        return false;
    struct timeval value = { .tv_sec = (time_t)saved, .tv_usec = 0 };
    return settimeofday(&value, NULL) == 0;
}
