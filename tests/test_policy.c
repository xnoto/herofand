#include <assert.h>

#include "config.h"
#include "policy.h"

int herofand_test_policy(void);

int herofand_test_policy(void) {
    const struct herofand_runtime_config config = herofand_default_config();
    struct herofand_channel_state state;
    int applied_tier;

    assert(herofand_curve_tier(&config.intake_curve, 44000) == 0);
    assert(herofand_curve_tier(&config.intake_curve, 55000) == 1);
    assert(herofand_curve_tier(&config.intake_curve, 65001) == 3);
    assert(herofand_curve_pwm(&config.gpu_curve, 4) == 255);

    herofand_channel_state_init(&state);
    assert(herofand_channel_state_apply(&state, 2, 100, 7, &applied_tier));
    assert(applied_tier == 2);

    assert(!herofand_channel_state_apply(&state, 1, 101, 7, &applied_tier));
    assert(!herofand_channel_state_apply(&state, 1, 107, 7, &applied_tier));
    assert(herofand_channel_state_apply(&state, 1, 108, 7, &applied_tier));
    assert(applied_tier == 1);

    assert(herofand_channel_state_apply(&state, 3, 109, 7, &applied_tier));
    assert(applied_tier == 3);

    return 0;
}
