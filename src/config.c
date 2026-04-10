#include "config.h"

static bool herofand_curve_is_valid(const struct herofand_curve *curve)
{
    if (curve == NULL) {
        return false;
    }

    if (curve->idle_temp_mc > curve->low_temp_mc) {
        return false;
    }
    if (curve->low_temp_mc > curve->med_temp_mc) {
        return false;
    }
    if (curve->med_temp_mc > curve->high_temp_mc) {
        return false;
    }
    if (curve->high_temp_mc > curve->max_temp_mc) {
        return false;
    }

    if (curve->pwm_idle < 0 || curve->pwm_max > 255) {
        return false;
    }
    if (curve->pwm_idle > curve->pwm_low) {
        return false;
    }
    if (curve->pwm_low > curve->pwm_med) {
        return false;
    }
    if (curve->pwm_med > curve->pwm_high) {
        return false;
    }
    if (curve->pwm_high > curve->pwm_max) {
        return false;
    }

    return true;
}

struct herofand_runtime_config herofand_default_config(void)
{
    return (struct herofand_runtime_config){
        .interval_seconds = 1.0,
        .downshift_delay_seconds = 7,
        .intake_curve = {
            .idle_temp_mc = 45000,
            .low_temp_mc = 55000,
            .med_temp_mc = 65000,
            .high_temp_mc = 70000,
            .max_temp_mc = 75000,
            .pwm_idle = 0,
            .pwm_low = 51,
            .pwm_med = 128,
            .pwm_high = 230,
            .pwm_max = 255,
        },
        .exhaust_curve = {
            .idle_temp_mc = 45000,
            .low_temp_mc = 60000,
            .med_temp_mc = 70000,
            .high_temp_mc = 75000,
            .max_temp_mc = 75000,
            .pwm_idle = 0,
            .pwm_low = 50,
            .pwm_med = 132,
            .pwm_high = 230,
            .pwm_max = 255,
        },
        .gpu_curve = {
            .idle_temp_mc = 55000,
            .low_temp_mc = 65000,
            .med_temp_mc = 69000,
            .high_temp_mc = 74000,
            .max_temp_mc = 74000,
            .pwm_idle = 0,
            .pwm_low = 51,
            .pwm_med = 128,
            .pwm_high = 230,
            .pwm_max = 255,
        },
        .verbose_logging = false,
    };
}

bool herofand_validate_config(const struct herofand_runtime_config *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->interval_seconds <= 0.0) {
        return false;
    }
    if (config->downshift_delay_seconds < 0) {
        return false;
    }

    return herofand_curve_is_valid(&config->intake_curve)
        && herofand_curve_is_valid(&config->exhaust_curve)
        && herofand_curve_is_valid(&config->gpu_curve);
}
