#ifndef HEROFAND_CONFIG_H
#define HEROFAND_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

enum herofand_case_pwm_channel {
    HEROFAND_CASE_PWM2 = 2,
    HEROFAND_CASE_PWM3 = 3,
    HEROFAND_CASE_PWM4 = 4,
    HEROFAND_PUMP_PWM7 = 7,
};

struct herofand_curve {
    int idle_temp_mc;
    int low_temp_mc;
    int med_temp_mc;
    int high_temp_mc;
    int max_temp_mc;
    int pwm_idle;
    int pwm_low;
    int pwm_med;
    int pwm_high;
    int pwm_max;
    int idle_dither_min_pwm;
    int idle_dither_max_pwm;
    int idle_dither_period_seconds;
    int idle_dither_dwell_seconds;
};

struct herofand_runtime_config {
    double interval_seconds;
    int downshift_delay_seconds;
    int gpu_idle_dither_index;
    struct herofand_curve intake_curve;
    struct herofand_curve exhaust_curve;
    struct herofand_curve gpu_curve;
    bool verbose_logging;
};

struct herofand_runtime_config herofand_default_config(void);
bool herofand_validate_config(const struct herofand_runtime_config *config);

#endif
