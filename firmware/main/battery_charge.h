#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Charging estimate from the battery voltage alone.  The board has no
 * charge-status line and no current sensor, so this follows the charger
 * through the two parts of a lithium charge, as they show in the voltage:
 *
 * - The climb.  The charger feeds a steady current and the voltage rises
 *   about 20 mV a minute.  Since the current is steady, the voltage says how
 *   far through the climb the battery is.
 * - The hold.  The charger holds the voltage at the top (about 4.3 V on this
 *   board's reading) while the current fades.  The voltage stops moving, so
 *   the time since the hold began is all there is to go on.
 *
 * When the charger finishes, the voltage falls back to the battery's own,
 * about 4.2 V.  The default figures come from a logged charge from empty:
 * about 24 minutes of climb and 42 of hold, 66 minutes in all.  After each
 * charge that runs to the end, the reader measures its own charger and
 * battery from the log and blends the result in, as it does for discharges.
 * The charge level assumes the current stays steady during the climb and
 * fades to a tenth of it during the hold.  Using the reader while it charges
 * takes some of the current, so the charge then runs a little slower than
 * estimated.
 *
 * A battery more than about 60 percent full has little or no climb: the
 * charger soon reaches the top and starts the hold part-way through.  So the
 * level the reader read just before the charger went on (the discharge
 * estimate, which the caller passes in each minute) says how far through the
 * hold the charge starts.  That level is only trusted from a reading taken
 * at least two minutes after start-up, as a reader that starts on the
 * charger reads the charger's lift from the first minute.
 *
 * Without that level, a climb of only a few minutes to the top gives
 * BATTERY_CHARGE_CHARGING, with no estimate.  A reading at the top with no
 * climb seen at all means the reader started on the charger or the battery
 * is switched off and the reading is the charger's own output:
 * BATTERY_CHARGE_ON_CHARGER, also with no estimate.
 *
 * Jumps.  Plugging the charger in lifts the reading about a third of a volt
 * within seconds, as the charging current pushes against the battery's
 * resistance; a real charge then rises only 13 to 20 mV a minute.  With the
 * charger in, the battery switch does not restart the reader, and flipping
 * it moves the reading just as suddenly: switched off, the reader reads the
 * charger's own output at the top; switched back on, the reading drops to
 * the battery's voltage plus the lift.  So the smoothing (below) follows a
 * jump at once, and the charge follows from what kind of jump it was:
 *
 * - up, while not charging: the charger went on.  The level read just
 *   before it is the battery's own.
 * - up to the top, while charging: the battery was switched off.  Not a
 *   charge to follow or learn; BATTERY_CHARGE_ON_CHARGER.
 * - down from the top: the battery was switched on with the charger in.
 *   The readings before were the charger's, so the level is unknown.
 *
 * Switching the battery off during the hold shows no jump, as the reading
 * is at the top either way.
 *
 * Plain C with no ESP-IDF calls, so it can be tested on a PC against a
 * battery log. */

/* At or above this the charger is holding the voltage.  A full battery at
 * rest reads about 4.20 V, and just after the charger finishes about
 * 4.26 V; the hold reads 4.28 to 4.32 V. */
#define BATTERY_CHARGE_HOLD_MV 4280

/* The smoothed cell voltage the estimate follows: each reading (one every
 * 3.2 seconds on the reader) weighted 1/32, which smooths over about two
 * minutes so a burst of speech does not move the level.  But a reading
 * BATTERY_CHARGE_JUMP_MV or more above the average three times running
 * (about ten seconds) is a jump, and the average takes it at once; so is
 * one that far below while the average is at the top.  Loads such as speech
 * move the reading about 10 mV, so a drop is only followed at the top,
 * where nothing but the battery switch causes one. */
#define BATTERY_CHARGE_JUMP_MV 100
#define BATTERY_SMOOTHING_SHIFT 5
typedef struct {
    int32_t scaled;  /* millivolts << BATTERY_SMOOTHING_SHIFT, 0 when empty */
    unsigned away;   /* readings in a row a jump away from the average */
} battery_smoothing_t;

/* Adds a reading and returns the smoothed millivolts. */
int battery_smoothing_add(battery_smoothing_t *smoothing, int millivolts);

typedef enum {
    BATTERY_CHARGE_NONE,       /* not charging, as far as the voltage shows */
    BATTERY_CHARGE_CLIMBING,
    BATTERY_CHARGE_HOLDING,
    BATTERY_CHARGE_ON_CHARGER, /* at the top with no way to tell how far */
    BATTERY_CHARGE_CHARGING,   /* charging, with no way to tell how far */
} battery_charge_phase_t;

typedef struct {
    battery_charge_phase_t phase;
    int recent_mv[4];      /* the last four minutes, for spotting the climb */
    int recent_level[4];   /* the discharge estimate at each of them */
    unsigned samples;
    int start_level;       /* level before the charger went on, or -1 */
    int hold_offset;       /* tenths of the hold already done at its start */
    int last_mv;
    int peak_mv;           /* highest voltage of the climb so far */
    unsigned peak_minute;
    unsigned climb_minute; /* when the climb began */
    unsigned hold_minute;  /* when the hold began */
    unsigned minute;       /* time of the newest update */
} battery_charge_t;

/* Call once a minute with the smoothed cell voltage, or 0 without a reading,
 * and the discharge estimate's level for it (0..100), or -1 if unknown.
 * Returns the phase. */
battery_charge_phase_t battery_charge_update(battery_charge_t *charge,
                                             unsigned minute, int millivolts,
                                             int level);

/* While climbing or holding, gives the estimated charge level (0..100) and
 * the minutes until the charger finishes, and returns true.  Minutes reach
 * 0 when the hold has run as long as expected; the charge is then nearly
 * done. */
bool battery_charge_estimate(const battery_charge_t *charge, int *percent,
                             int *minutes_to_full);

/* The figures the estimate uses, in tenths of a minute, as seen on the
 * reader's smoothed voltage: the time from each voltage in
 * battery_charge_curve_mv to the start of the hold, and the hold itself. */
#define BATTERY_CHARGE_POINTS 11
extern const int16_t battery_charge_curve_mv[BATTERY_CHARGE_POINTS];
typedef struct {
    uint16_t climb_left[BATTERY_CHARGE_POINTS];
    uint16_t hold;
} battery_charge_curve_t;

void battery_charge_default_curve(battery_charge_curve_t *curve);
bool battery_charge_curve_valid(const battery_charge_curve_t *curve);
/* Call before the first battery_charge_estimate. */
void battery_charge_use_curve(const battery_charge_curve_t *curve);

/* Learning.  Feed one log session's readings in order, with the minute
 * average each record holds; minutes between records repeat the last, as
 * the log only writes on a change.  The learner smooths them as the reader
 * does, follows the charge as battery_charge_update does, and reports a
 * charge once it has seen it finish.
 *
 * A charge counts as finished, rather than unplugged, when the voltage stays
 * at 4.2 V or above for ten minutes after the hold ends: a battery taken off
 * the charger part-way through the hold falls below that.  On this board,
 * unplugging the charger restarts the reader at once, so a session that
 * ends after the hold has ended, still at 4.2 V or above, counts too; one
 * unplugged during the hold ends in the hold (call
 * battery_charge_learner_end at the end of each session).  The hold
 * is only measured after a climb of eight minutes or more: a battery that
 * reaches the top sooner started too full for the whole hold. */
typedef struct {
    bool climb_covered[BATTERY_CHARGE_POINTS];
    uint16_t climb_left[BATTERY_CHARGE_POINTS];
    bool hold_covered;
    uint16_t hold;
    int start_mv;          /* smoothed voltage when the climb was seen */
    uint32_t end_sequence; /* the record that showed the charge finished */
} battery_charge_measured_t;

typedef struct {
    battery_charge_t state;
    battery_smoothing_t smoothing; /* as the reader's average */
    int mv;             /* the newest minute average */
    unsigned minute;    /* the newest minute fed */
    uint32_t sequence;  /* the newest record fed */
    bool started;
    int previous;       /* smoothed voltage a minute before */
    int crossed[BATTERY_CHARGE_POINTS]; /* tenths, or -1 */
    int start_mv;
    int finish;         /* tenths, when the hold ended */
    bool checking;      /* watching the ten minutes after the hold */
    unsigned check_until;
    int lowest_after;
} battery_charge_learner_t;

void battery_charge_learner_reset(battery_charge_learner_t *learner);
/* Returns true, filling *measured, when this record completes a finished
 * charge. */
bool battery_charge_learner_feed(battery_charge_learner_t *learner,
                                 unsigned minute, int millivolts,
                                 uint32_t sequence,
                                 battery_charge_measured_t *measured);
/* At the end of a session: returns true, filling *measured, if a charge had
 * finished and the session ended while confirming it. */
bool battery_charge_learner_end(battery_charge_learner_t *learner,
                                battery_charge_measured_t *measured);
/* Blends a measured charge into the curve: the first replaces the default,
 * the next counts for a half, and each after that for a third.  *runs counts
 * the charges learned. */
void battery_charge_learn(battery_charge_curve_t *curve, unsigned *runs,
                          const battery_charge_measured_t *measured);
