#include <stddef.h>

#include "policy.h"

int herofand_curve_tier(const struct herofand_curve *curve, int temp_mc) {
    if (curve == NULL) {
        return 0;
    }

    if (temp_mc <= curve->idle_temp_mc) {
        return 0;
    }
    if (temp_mc <= curve->low_temp_mc) {
        return 1;
    }
    if (temp_mc <= curve->med_temp_mc) {
        return 2;
    }
    if (temp_mc <= curve->high_temp_mc) {
        return 3;
    }
    return 4;
}

int herofand_curve_pwm(const struct herofand_curve *curve, int tier) {
    if (curve == NULL) {
        return 0;
    }

    switch (tier) {
    case 0:
        return curve->pwm_idle;
    case 1:
        return curve->pwm_low;
    case 2:
        return curve->pwm_med;
    case 3:
        return curve->pwm_high;
    default:
        return curve->pwm_max;
    }
}

int herofand_curve_idle_pwm(const struct herofand_curve *curve, long idle_dwell_seconds) {
    long period;
    long half;
    long t;
    int range;
    int offset;

    if (curve == NULL) {
        return 0;
    }

    if (curve->idle_dither_period_seconds <= 0 ||
        idle_dwell_seconds < (long)curve->idle_dither_dwell_seconds) {
        return curve->pwm_idle;
    }

    period = (long)curve->idle_dither_period_seconds;
    half = period / 2;
    if (half <= 0) {
        return curve->pwm_idle;
    }

    t = (idle_dwell_seconds - (long)curve->idle_dither_dwell_seconds) % period;
    range = curve->idle_dither_max_pwm - curve->idle_dither_min_pwm;

    if (t < half) {
        offset = (int)((t * (long)range) / half);
    } else {
        offset = (int)(((period - t) * (long)range) / (period - half));
    }

    return curve->idle_dither_min_pwm + offset;
}

void herofand_channel_state_init(struct herofand_channel_state *state) {
    if (state == NULL) {
        return;
    }

    state->last_tier = -1;
    state->down_target = -1;
    state->down_since_seconds = 0;
    state->idle_entered_seconds = 0;
}

bool herofand_channel_state_apply(struct herofand_channel_state *state, int new_tier,
                                  long now_seconds, int downshift_delay_seconds,
                                  int *applied_tier) {
    int prev_tier;
    bool changed = false;

    if (state == NULL || applied_tier == NULL) {
        return false;
    }

    prev_tier = state->last_tier;

    if (state->last_tier < 0 || new_tier > state->last_tier) {
        state->last_tier = new_tier;
        state->down_target = -1;
        state->down_since_seconds = 0;
        *applied_tier = new_tier;
        changed = true;
    } else if (new_tier < state->last_tier) {
        if (state->down_target != new_tier) {
            state->down_target = new_tier;
            state->down_since_seconds = now_seconds;
        } else if ((now_seconds - state->down_since_seconds) >= downshift_delay_seconds) {
            state->last_tier = new_tier;
            state->down_target = -1;
            state->down_since_seconds = 0;
            *applied_tier = new_tier;
            changed = true;
        }
    } else {
        state->down_target = -1;
        state->down_since_seconds = 0;
    }

    if (changed && state->last_tier == 0 && prev_tier != 0) {
        state->idle_entered_seconds = now_seconds;
    }

    return changed;
}
