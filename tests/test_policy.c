#include <assert.h>

#include "config.h"
#include "policy.h"

int herofand_test_policy(void);

int herofand_test_policy(void) {
    const struct herofand_runtime_config config = herofand_default_config();
    struct herofand_channel_state state;
    int applied_tier;

    assert(herofand_curve_tier(&config.intake_curve, 60000) == 0);
    assert(herofand_curve_tier(&config.intake_curve, 72000) == 1);
    assert(herofand_curve_tier(&config.intake_curve, 85000) == 3);
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

    {
        struct herofand_curve disabled = {0};
        disabled.pwm_idle = 0;
        disabled.idle_dither_period_seconds = 0;
        assert(herofand_curve_idle_pwm(&disabled, 9999) == 0);
    }

    {
        struct herofand_curve dither = {0};
        dither.pwm_idle = 32;
        dither.idle_dither_min_pwm = 0;
        dither.idle_dither_max_pwm = 160;
        dither.idle_dither_period_seconds = 180;
        dither.idle_dither_dwell_seconds = 30;

        assert(herofand_curve_idle_pwm(&dither, 0) == 32);
        assert(herofand_curve_idle_pwm(&dither, 29) == 32);
        assert(herofand_curve_idle_pwm(&dither, 30) == 0);
        assert(herofand_curve_idle_pwm(&dither, 75) == 80);
        assert(herofand_curve_idle_pwm(&dither, 120) == 160);
        assert(herofand_curve_idle_pwm(&dither, 165) == 80);
        assert(herofand_curve_idle_pwm(&dither, 210) == 0);
        assert(herofand_curve_idle_pwm(&dither, 300) == 160);
    }

    {
        struct herofand_channel_state idle_state;
        herofand_channel_state_init(&idle_state);
        assert(idle_state.idle_entered_seconds == 0);

        assert(herofand_channel_state_apply(&idle_state, 0, 500, 7, &applied_tier));
        assert(applied_tier == 0);
        assert(idle_state.idle_entered_seconds == 500);

        assert(herofand_channel_state_apply(&idle_state, 2, 600, 7, &applied_tier));
        assert(idle_state.last_tier == 2);

        assert(!herofand_channel_state_apply(&idle_state, 0, 700, 7, &applied_tier));
        assert(herofand_channel_state_apply(&idle_state, 0, 707, 7, &applied_tier));
        assert(idle_state.idle_entered_seconds == 707);
    }

    return 0;
}
