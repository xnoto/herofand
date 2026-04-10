#ifndef HEROFAND_POLICY_H
#define HEROFAND_POLICY_H

#include <stdbool.h>

#include "config.h"

struct herofand_channel_state {
    int last_tier;
    int down_target;
    long down_since_seconds;
};

int herofand_curve_tier(const struct herofand_curve *curve, int temp_mc);
int herofand_curve_pwm(const struct herofand_curve *curve, int tier);

void herofand_channel_state_init(struct herofand_channel_state *state);
bool herofand_channel_state_apply(struct herofand_channel_state *state, int new_tier,
                                  long now_seconds, int downshift_delay_seconds, int *applied_tier);

#endif
