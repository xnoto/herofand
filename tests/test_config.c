#include <assert.h>

#include "config.h"

int herofand_test_config(void);

int herofand_test_config(void)
{
    struct herofand_runtime_config config = herofand_default_config();

    assert(herofand_validate_config(&config));

    config.interval_seconds = 0.0;
    assert(!herofand_validate_config(&config));

    config = herofand_default_config();
    config.gpu_curve.pwm_max = 300;
    assert(!herofand_validate_config(&config));

    config = herofand_default_config();
    config.intake_curve.low_temp_mc = 44000;
    assert(!herofand_validate_config(&config));

    return 0;
}
