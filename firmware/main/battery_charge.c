#include "battery_charge.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define CHARGE_HOLD_MV BATTERY_CHARGE_HOLD_MV
/* A rise this large over three minutes (or over the minutes there are, just
   after start-up) is the climb rather than the battery recovering after
   playback pauses. */
#define CHARGE_RISE_MV 40
/* The climb ends without reaching the hold if the voltage drops this far
   below its highest, which is the charger being unplugged, or makes no new
   high for this long. */
#define CHARGE_DROP_MV 30
#define CHARGE_STALL_MINUTES 5
#define CHARGE_PEAK_STEP_MV 5
/* A climb to the hold this quick is not a charge to follow: the reading
   jumps that fast when the battery is switched off and the reader reads the
   charger, or when a nearly full battery goes on the charger. */
#define CHARGE_MIN_CLIMB_MINUTES 8
/* A hold that runs this long past the expected length is not a charge, but
   the battery switched off with the reader running from USB. */
#define CHARGE_HOLD_OVERRUN_MINUTES 30

/* Learning: after the hold, a finished charge stays at or above this for
   this long; the hold is measured only after a climb of at least
   CHARGE_MIN_CLIMB_MINUTES; and measurements outside these bounds, in
   tenths of a minute, are ignored. */
#define CHARGE_FINISHED_MV 4200
#define CHARGE_FINISHED_MINUTES 10
#define CHARGE_HOLD_MIN_TENTHS 150
#define CHARGE_HOLD_MAX_TENTHS 1200
#define CHARGE_CLIMB_MAX_TENTHS 1200

const int16_t battery_charge_curve_mv[BATTERY_CHARGE_POINTS] = {
    3600, 3800, 3900, 3950, 4000, 4050, 4100, 4150, 4200, 4250, CHARGE_HOLD_MV,
};

/* From the logged charge from empty, timed on the reader's smoothed
   voltage, which lags the climb by about two minutes: the hold is seen to
   begin about 25 minutes in and lasts about 40 more. */
#define DEFAULT_CURVE { \
    .climb_left = { 250, 222, 198, 178, 155, 130, 108, 88, 64, 30, 0 }, \
    .hold = 402, \
}
static const battery_charge_curve_t default_curve = DEFAULT_CURVE;
static battery_charge_curve_t curve = DEFAULT_CURVE;

/* Readings in a row a jump away before the average takes them. */
#define SMOOTHING_JUMP_READINGS 3

int battery_smoothing_add(battery_smoothing_t *smoothing, int millivolts)
{
    int average = (int)(smoothing->scaled >> BATTERY_SMOOTHING_SHIFT);
    int away = millivolts - average;
    if (smoothing->scaled
        && (away >= BATTERY_CHARGE_JUMP_MV
            || (away <= -BATTERY_CHARGE_JUMP_MV && average >= CHARGE_HOLD_MV))) {
        if (++smoothing->away >= SMOOTHING_JUMP_READINGS) smoothing->scaled = 0;
    } else {
        smoothing->away = 0;
    }
    /* Fixed-point exponential average, so small steps still accumulate. */
    if (!smoothing->scaled) {
        smoothing->scaled = (int32_t)millivolts << BATTERY_SMOOTHING_SHIFT;
        smoothing->away = 0;
    } else {
        smoothing->scaled += millivolts
            - (smoothing->scaled >> BATTERY_SMOOTHING_SHIFT);
    }
    return (int)(smoothing->scaled >> BATTERY_SMOOTHING_SHIFT);
}

void battery_charge_default_curve(battery_charge_curve_t *out)
{
    *out = default_curve;
}

bool battery_charge_curve_valid(const battery_charge_curve_t *candidate)
{
    if (candidate->climb_left[BATTERY_CHARGE_POINTS - 1] != 0
        || !candidate->climb_left[0]
        || candidate->climb_left[0] > CHARGE_CLIMB_MAX_TENTHS
        || candidate->hold < CHARGE_HOLD_MIN_TENTHS
        || candidate->hold > CHARGE_HOLD_MAX_TENTHS)
        return false;
    for (size_t i = 1; i < BATTERY_CHARGE_POINTS; ++i)
        if (candidate->climb_left[i] > candidate->climb_left[i - 1])
            return false;
    return true;
}

void battery_charge_use_curve(const battery_charge_curve_t *chosen)
{
    if (battery_charge_curve_valid(chosen)) curve = *chosen;
}

static int climb_tenths_left(int millivolts)
{
    const int16_t *mv = battery_charge_curve_mv;
    const uint16_t *left = curve.climb_left;
    if (millivolts <= mv[0]) return left[0];
    for (size_t i = 1; i < BATTERY_CHARGE_POINTS; ++i) {
        if (millivolts > mv[i]) continue;
        return left[i] + (mv[i] - millivolts) * (left[i - 1] - left[i])
            / (mv[i] - mv[i - 1]);
    }
    return 0;
}

/* The model behind the charge level: the climb's steady current, then the
   hold's current fading to a tenth, e^(-hold/tau) = 1/10.  Charge is counted
   in tenths of a minute at the climb's current. */
static float hold_tau(void) { return curve.hold / 2.302585f; }
static float charge_total(void)
{
    return curve.climb_left[0] + 0.9f * hold_tau();
}

/* How far into the hold, in tenths of a minute, a charge that starts at this
   level begins: none below the level the hold starts at. */
static int hold_offset_for(int level)
{
    float tau = hold_tau();
    float x = (level * charge_total() / 100.0f - curve.climb_left[0]) / tau;
    if (x <= 0.0f) return 0;
    if (x >= 0.9f) return curve.hold;
    int offset = (int)(-tau * logf(1.0f - x) + 0.5f);
    return offset > curve.hold ? curve.hold : offset;
}

battery_charge_phase_t battery_charge_update(battery_charge_t *charge,
                                             unsigned minute, int millivolts,
                                             int level)
{
    charge->minute = minute;
    if (!millivolts) return charge->phase;
    /* The oldest of up to three minutes back.  Its level is trusted as the
       level before the charger went on only from the third minute after
       start-up: a reader that starts on the charger reads its lift at
       once, and sees the climb within the first few minutes. */
    unsigned back = charge->samples < 3 ? charge->samples : 3;
    unsigned before_index = charge->samples - back;
    int before = back ? charge->recent_mv[before_index % 4] : 0;
    int before_level = back && before_index >= 2
        ? charge->recent_level[before_index % 4] : -1;
    bool rising = before && millivolts - before >= CHARGE_RISE_MV;
    /* The smoothing takes a jump at once, so the voltage only moves this
       far in a minute when the charger or the battery switch did it. */
    int jump = charge->last_mv ? millivolts - charge->last_mv : 0;
    int last_level = charge->samples
        ? charge->recent_level[(charge->samples - 1) % 4] : -1;
    bool charging = charge->phase == BATTERY_CHARGE_CLIMBING
        || charge->phase == BATTERY_CHARGE_HOLDING
        || charge->phase == BATTERY_CHARGE_CHARGING;

    if (jump >= BATTERY_CHARGE_JUMP_MV && charging
        && millivolts >= CHARGE_HOLD_MV) {
        /* The battery was switched off: the reading is the charger's. */
        charge->phase = BATTERY_CHARGE_ON_CHARGER;
    } else if (jump >= BATTERY_CHARGE_JUMP_MV
               && charge->phase == BATTERY_CHARGE_NONE) {
        /* The charger went on.  The level a minute ago was the battery's
           own; a nearly full battery jumps straight to the hold. */
        charge->start_level = last_level;
        charge->hold_offset = last_level >= 0 ? hold_offset_for(last_level) : 0;
        charge->peak_mv = millivolts;
        charge->peak_minute = charge->climb_minute = minute;
        if (millivolts < CHARGE_HOLD_MV) {
            charge->phase = BATTERY_CHARGE_CLIMBING;
        } else {
            charge->phase = last_level >= 0 ? BATTERY_CHARGE_HOLDING
                                            : BATTERY_CHARGE_CHARGING;
            charge->hold_minute = minute;
        }
    } else if (jump <= -BATTERY_CHARGE_JUMP_MV
               && charge->last_mv >= CHARGE_HOLD_MV) {
        /* The battery was switched on with the charger in.  The readings
           before were the charger's own, so the level is unknown. */
        charge->phase = BATTERY_CHARGE_CLIMBING;
        charge->start_level = -1;
        charge->hold_offset = 0;
        charge->peak_mv = millivolts;
        charge->peak_minute = charge->climb_minute = minute;
    } else if (millivolts >= CHARGE_HOLD_MV) {
        if (charge->phase == BATTERY_CHARGE_CLIMBING) {
            /* A short climb from an unknown level could be the battery
               switched off as well as a part-full battery charging. */
            bool known = charge->start_level >= 0
                || minute - charge->climb_minute >= CHARGE_MIN_CLIMB_MINUTES;
            charge->phase = known ? BATTERY_CHARGE_HOLDING
                                  : BATTERY_CHARGE_CHARGING;
            charge->hold_minute = minute;
        } else if (charge->phase == BATTERY_CHARGE_HOLDING
                   || charge->phase == BATTERY_CHARGE_CHARGING) {
            int offset = charge->phase == BATTERY_CHARGE_HOLDING
                ? charge->hold_offset : 0;
            if ((int)(minute - charge->hold_minute) * 10 + offset
                > curve.hold + CHARGE_HOLD_OVERRUN_MINUTES * 10)
                charge->phase = BATTERY_CHARGE_ON_CHARGER;
        } else {
            charge->phase = BATTERY_CHARGE_ON_CHARGER;
        }
    } else if (charge->phase == BATTERY_CHARGE_HOLDING
               || charge->phase == BATTERY_CHARGE_CHARGING
               || charge->phase == BATTERY_CHARGE_ON_CHARGER) {
        /* The charger finished, or was unplugged. */
        charge->phase = BATTERY_CHARGE_NONE;
    } else if (charge->phase == BATTERY_CHARGE_CLIMBING) {
        if (millivolts >= charge->peak_mv + CHARGE_PEAK_STEP_MV) {
            charge->peak_mv = millivolts;
            charge->peak_minute = minute;
        } else if (millivolts < charge->peak_mv - CHARGE_DROP_MV
                   || minute - charge->peak_minute >= CHARGE_STALL_MINUTES) {
            charge->phase = BATTERY_CHARGE_NONE;
        }
    } else if (rising) {
        charge->phase = BATTERY_CHARGE_CLIMBING;
        charge->peak_mv = millivolts;
        charge->peak_minute = charge->climb_minute = minute;
        charge->start_level = before_level;
        charge->hold_offset = before_level >= 0
            ? hold_offset_for(before_level) : 0;
    }

    charge->recent_level[charge->samples % 4] = level;
    charge->recent_mv[charge->samples % 4] = millivolts;
    ++charge->samples;
    charge->last_mv = millivolts;
    return charge->phase;
}

bool battery_charge_estimate(const battery_charge_t *charge, int *percent,
                             int *minutes_to_full)
{
    const int climb_total = curve.climb_left[0], hold = curve.hold;
    const float tau = hold_tau(), total = charge_total();
    float done;
    int left;
    if (charge->phase == BATTERY_CHARGE_CLIMBING && charge->hold_offset > 0) {
        /* Started part full: the short climb left is the start of the
           hold, as far as the estimate goes. */
        int held = charge->hold_offset;
        done = climb_total + tau * (1.0f - expf(-held / tau));
        left = hold - held;
    } else if (charge->phase == BATTERY_CHARGE_CLIMBING) {
        int climb = climb_tenths_left(charge->last_mv);
        done = (float)(climb_total - climb);
        left = climb + hold;
    } else if (charge->phase == BATTERY_CHARGE_HOLDING) {
        int held = (int)(charge->minute - charge->hold_minute) * 10
            + charge->hold_offset;
        if (held > hold) held = hold;
        done = climb_total + tau * (1.0f - expf(-held / tau));
        left = hold - held;
    } else {
        return false;
    }
    int level = (int)(done * 100.0f / total + 0.5f);
    *percent = level < 0 ? 0 : level > 100 ? 100 : level;
    *minutes_to_full = (left + 5) / 10;
    return true;
}

void battery_charge_learner_reset(battery_charge_learner_t *learner)
{
    memset(learner, 0, sizeof(*learner));
}

/* Where the smoothed voltage crossed `mv` during the minute just stepped,
   in tenths of a minute, assuming a straight line between the minutes. */
static int crossing(unsigned minute, int previous, int now, int mv)
{
    return (int)(minute - 1) * 10 + (mv - previous) * 10 / (now - previous);
}

static bool learner_result(const battery_charge_learner_t *learner,
                           battery_charge_measured_t *measured);

static bool learner_step(battery_charge_learner_t *learner, unsigned minute,
                         int millivolts, battery_charge_measured_t *measured)
{
    /* The reader averages a reading every 3.2 seconds, about 19 a minute. */
    int now = 0;
    for (int i = 0; i < 19; ++i)
        now = battery_smoothing_add(&learner->smoothing, millivolts);
    int previous = learner->previous;
    battery_charge_phase_t before = learner->state.phase;
    battery_charge_phase_t after =
        battery_charge_update(&learner->state, minute, now, -1);
    learner->previous = now;
    bool done = false;

    if (after == BATTERY_CHARGE_CLIMBING && before != BATTERY_CHARGE_CLIMBING) {
        for (size_t i = 0; i < BATTERY_CHARGE_POINTS; ++i)
            learner->crossed[i] = -1;
        learner->start_mv = now;
        learner->checking = false;
    }
    /* Voltages passed in a jump were not climbed through. */
    if (before == BATTERY_CHARGE_CLIMBING || after == BATTERY_CHARGE_CLIMBING) {
        if ((after == BATTERY_CHARGE_CLIMBING
             || after == BATTERY_CHARGE_HOLDING) && previous && now > previous
            && now - previous < BATTERY_CHARGE_JUMP_MV)
            for (size_t i = 0; i < BATTERY_CHARGE_POINTS; ++i) {
                int mv = battery_charge_curve_mv[i];
                if (learner->crossed[i] < 0 && previous < mv && now >= mv)
                    learner->crossed[i] = crossing(minute, previous, now, mv);
            }
    }
    if (before == BATTERY_CHARGE_HOLDING && after == BATTERY_CHARGE_NONE
        && previous > now) {
        learner->finish = crossing(minute, previous, now, CHARGE_HOLD_MV);
        learner->checking = true;
        learner->check_until = minute + CHARGE_FINISHED_MINUTES;
        learner->lowest_after = millivolts;
    } else if (learner->checking) {
        if (millivolts < learner->lowest_after)
            learner->lowest_after = millivolts;
        if (learner->state.phase != BATTERY_CHARGE_NONE) {
            learner->checking = false;
        } else if (minute >= learner->check_until) {
            learner->checking = false;
            done = learner->lowest_after >= CHARGE_FINISHED_MV;
        }
    }
    return done && learner_result(learner, measured);
}

/* At the end of a session: on this board unplugging the charger restarts
   the reader at once, so a hold that ended within the session, with the
   voltage still at 4.2 V or above when the session ended, was a finished
   charge too.  Unplugging during the hold ends the session in the hold. */
bool battery_charge_learner_end(battery_charge_learner_t *learner,
                                battery_charge_measured_t *measured)
{
    bool finished = learner->checking
        && learner->lowest_after >= CHARGE_FINISHED_MV
        && learner_result(learner, measured);
    if (finished) measured->end_sequence = learner->sequence;
    battery_charge_learner_reset(learner);
    return finished;
}

static bool learner_result(const battery_charge_learner_t *learner,
                           battery_charge_measured_t *measured)
{
    int hold_start = learner->crossed[BATTERY_CHARGE_POINTS - 1];
    if (hold_start < 0) return false;
    memset(measured, 0, sizeof(*measured));
    measured->start_mv = learner->start_mv;
    for (size_t i = 0; i < BATTERY_CHARGE_POINTS; ++i) {
        int left = hold_start - learner->crossed[i];
        if (learner->crossed[i] < 0 || left < 0
            || left > CHARGE_CLIMB_MAX_TENTHS)
            continue;
        measured->climb_covered[i] = true;
        measured->climb_left[i] = (uint16_t)left;
    }
    int hold = learner->finish - hold_start;
    int climbed = hold_start - (int)learner->state.climb_minute * 10;
    if (climbed >= CHARGE_MIN_CLIMB_MINUTES * 10
        && hold >= CHARGE_HOLD_MIN_TENTHS && hold <= CHARGE_HOLD_MAX_TENTHS) {
        measured->hold_covered = true;
        measured->hold = (uint16_t)hold;
    }
    return true;
}

bool battery_charge_learner_feed(battery_charge_learner_t *learner,
                                 unsigned minute, int millivolts,
                                 uint32_t sequence,
                                 battery_charge_measured_t *measured)
{
    bool found = false;
    if (!learner->started) {
        learner->started = true;
        learner->minute = minute;
        learner->mv = millivolts;
        found = learner_step(learner, minute, millivolts, measured);
    } else {
        /* Minutes with no record repeat the last; a long gap means records
           were lost, so only the last hour of it is stepped through. */
        unsigned from = learner->minute + 1;
        if (minute > from + 60) from = minute - 60;
        for (unsigned m = from; m <= minute; ++m)
            if (learner_step(learner, m, m < minute ? learner->mv : millivolts,
                             measured))
                found = true;
        learner->minute = minute;
        learner->mv = millivolts;
    }
    learner->sequence = sequence;
    if (found) measured->end_sequence = sequence;
    return found;
}

void battery_charge_learn(battery_charge_curve_t *target, unsigned *runs,
                          const battery_charge_measured_t *measured)
{
    int top = -1;
    for (size_t i = 0; i < BATTERY_CHARGE_POINTS; ++i)
        if (measured->climb_covered[i]) {
            top = (int)i;
            break;
        }
    if (top < 0 && !measured->hold_covered) return;
    int divisor = *runs < 2 ? (int)*runs + 1 : 3;
    if (top >= 0) {
        int old_top = target->climb_left[top];
        for (size_t i = top; i < BATTERY_CHARGE_POINTS; ++i) {
            if (!measured->climb_covered[i]) continue;
            int old = target->climb_left[i];
            target->climb_left[i] =
                (uint16_t)(old + ((int)measured->climb_left[i] - old) / divisor);
        }
        /* Voltages below where the climb was first seen keep their shape,
           scaled to the new value where it was. */
        for (int i = 0; i < top; ++i)
            target->climb_left[i] = (uint16_t)(old_top
                ? target->climb_left[i] * target->climb_left[top] / old_top
                : target->climb_left[top]);
        target->climb_left[BATTERY_CHARGE_POINTS - 1] = 0;
        for (int i = BATTERY_CHARGE_POINTS - 2; i >= 0; --i)
            if (target->climb_left[i] < target->climb_left[i + 1])
                target->climb_left[i] = target->climb_left[i + 1];
    }
    if (measured->hold_covered)
        target->hold = (uint16_t)(target->hold
            + ((int)measured->hold - (int)target->hold) / divisor);
    ++*runs;
}
