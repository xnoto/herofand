#include <errno.h>
#include <glob.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "controller.h"
#include "policy.h"
#include "sysfs.h"

#define HEROFAND_NCT_NAME "nct6798"
#define HEROFAND_GPU_NAME "amdgpu"

static const char *const herofand_excluded_labels[] = {
    "AUXTIN1",
    "AUXTIN2",
    "PCH_CHIP_CPU_MAX_TEMP",
    "PCH_CHIP_TEMP",
    "PCH_CPU_TEMP",
};

struct herofand_sensor {
    char *path;
    char *label;
    char *hwmon_name;
};

struct herofand_sensor_list {
    struct herofand_sensor *items;
    size_t count;
};

struct herofand_gpu {
    char *hwmon_path;
    struct herofand_sensor_list sensors;
    struct herofand_channel_state state;
};

struct herofand_gpu_list {
    struct herofand_gpu *items;
    size_t count;
};

struct herofand_runtime {
    char *nct_path;
    struct herofand_sensor_list system_sensors;
    struct herofand_gpu_list gpus;
    struct herofand_channel_state intake_state;
    struct herofand_channel_state exhaust_state;
};

static volatile sig_atomic_t herofand_running = 1;

static void herofand_handle_signal(int signum)
{
    (void)signum;
    herofand_running = 0;
}

static void herofand_free_sensor(struct herofand_sensor *sensor)
{
    if (sensor == NULL) {
        return;
    }

    free(sensor->path);
    free(sensor->label);
    free(sensor->hwmon_name);
}

static void herofand_free_sensor_list(struct herofand_sensor_list *list)
{
    size_t i;

    if (list == NULL) {
        return;
    }

    for (i = 0; i < list->count; ++i) {
        herofand_free_sensor(&list->items[i]);
    }

    free(list->items);
    list->items = NULL;
    list->count = 0U;
}

static void herofand_free_runtime(struct herofand_runtime *runtime)
{
    size_t i;

    if (runtime == NULL) {
        return;
    }

    free(runtime->nct_path);
    herofand_free_sensor_list(&runtime->system_sensors);
    for (i = 0; i < runtime->gpus.count; ++i) {
        free(runtime->gpus.items[i].hwmon_path);
        herofand_free_sensor_list(&runtime->gpus.items[i].sensors);
    }
    free(runtime->gpus.items);
    runtime->gpus.items = NULL;
    runtime->gpus.count = 0U;
}

static bool herofand_label_is_excluded(const char *label)
{
    size_t i;

    if (label == NULL) {
        return false;
    }

    for (i = 0; i < (sizeof(herofand_excluded_labels) / sizeof(herofand_excluded_labels[0])); ++i) {
        if (strcmp(label, herofand_excluded_labels[i]) == 0) {
            return true;
        }
    }

    return false;
}

static char *herofand_strdup_string(const char *text)
{
    size_t len;
    char *copy;

    if (text == NULL) {
        return NULL;
    }

    len = strlen(text) + 1U;
    copy = malloc(len);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, len);
    return copy;
}

static bool herofand_append_sensor(struct herofand_sensor_list *list,
                                   const char *path,
                                   const char *label,
                                   const char *hwmon_name)
{
    struct herofand_sensor *next;

    if (list == NULL || path == NULL || hwmon_name == NULL) {
        return false;
    }

    next = realloc(list->items, (list->count + 1U) * sizeof(*next));
    if (next == NULL) {
        return false;
    }

    list->items = next;
    list->items[list->count].path = herofand_strdup_string(path);
    list->items[list->count].label = herofand_strdup_string(label != NULL ? label : "(no label)");
    list->items[list->count].hwmon_name = herofand_strdup_string(hwmon_name);
    if (list->items[list->count].path == NULL
        || list->items[list->count].label == NULL
        || list->items[list->count].hwmon_name == NULL) {
        herofand_free_sensor(&list->items[list->count]);
        return false;
    }

    list->count += 1U;
    return true;
}

static bool herofand_append_gpu(struct herofand_gpu_list *list, struct herofand_gpu *gpu)
{
    struct herofand_gpu *next;

    if (list == NULL || gpu == NULL) {
        return false;
    }

    next = realloc(list->items, (list->count + 1U) * sizeof(*next));
    if (next == NULL) {
        return false;
    }

    list->items = next;
    list->items[list->count] = *gpu;
    list->count += 1U;
    return true;
}

static bool herofand_find_nct(struct herofand_runtime *runtime)
{
    glob_t matches;
    size_t i;
    bool found = false;

    memset(&matches, 0, sizeof(matches));
    if (glob("/sys/class/hwmon/hwmon*", 0, NULL, &matches) != 0) {
        return false;
    }

    for (i = 0; i < matches.gl_pathc; ++i) {
        char *name_path;
        char buffer[64];

        name_path = herofand_path_join(matches.gl_pathv[i], "name");
        if (name_path == NULL) {
            continue;
        }

        if (herofand_read_string(name_path, buffer, sizeof(buffer))
            && strcmp(buffer, HEROFAND_NCT_NAME) == 0) {
            runtime->nct_path = herofand_strdup_string(matches.gl_pathv[i]);
            found = runtime->nct_path != NULL;
            free(name_path);
            break;
        }

        free(name_path);
    }

    globfree(&matches);
    return found;
}

static bool herofand_collect_temp_inputs(const char *hwmon_path, glob_t *matches)
{
    char *pattern;
    int result;

    if (hwmon_path == NULL || matches == NULL) {
        return false;
    }

    pattern = herofand_path_join(hwmon_path, "temp*_input");
    if (pattern == NULL) {
        return false;
    }

    memset(matches, 0, sizeof(*matches));
    result = glob(pattern, 0, NULL, matches);
    free(pattern);

    return result == 0;
}

static bool herofand_discover(struct herofand_runtime *runtime)
{
    glob_t hwmons;
    size_t i;

    if (!herofand_find_nct(runtime)) {
        fprintf(stderr, "ERROR: %s not found\n", HEROFAND_NCT_NAME);
        return false;
    }

    memset(&hwmons, 0, sizeof(hwmons));
    if (glob("/sys/class/hwmon/hwmon*", 0, NULL, &hwmons) != 0) {
        return false;
    }

    for (i = 0; i < hwmons.gl_pathc; ++i) {
        char *name_path;
        char hwmon_name[64];
        bool is_gpu = false;
        glob_t temps;
        size_t j;
        struct herofand_gpu gpu;

        memset(&gpu, 0, sizeof(gpu));
        name_path = herofand_path_join(hwmons.gl_pathv[i], "name");
        if (name_path == NULL) {
            continue;
        }

        if (!herofand_read_string(name_path, hwmon_name, sizeof(hwmon_name))) {
            free(name_path);
            continue;
        }
        free(name_path);

        if (strcmp(hwmon_name, HEROFAND_GPU_NAME) == 0) {
            char *pwm1;
            char *pwm1_enable;

            pwm1 = herofand_path_join(hwmons.gl_pathv[i], "pwm1");
            pwm1_enable = herofand_path_join(hwmons.gl_pathv[i], "pwm1_enable");
            if (pwm1 != NULL && pwm1_enable != NULL) {
                is_gpu = herofand_path_exists(pwm1) && herofand_path_exists(pwm1_enable);
            }
            free(pwm1);
            free(pwm1_enable);
        }

        if (!herofand_collect_temp_inputs(hwmons.gl_pathv[i], &temps)) {
            continue;
        }

        if (is_gpu) {
            gpu.hwmon_path = herofand_strdup_string(hwmons.gl_pathv[i]);
            if (gpu.hwmon_path == NULL) {
                globfree(&temps);
                globfree(&hwmons);
                return false;
            }

            herofand_channel_state_init(&gpu.state);
            for (j = 0; j < temps.gl_pathc; ++j) {
                if (!herofand_append_sensor(&gpu.sensors, temps.gl_pathv[j], NULL, hwmon_name)) {
                    free(gpu.hwmon_path);
                    herofand_free_sensor_list(&gpu.sensors);
                    globfree(&temps);
                    globfree(&hwmons);
                    return false;
                }
            }

            if (gpu.sensors.count > 0U) {
                if (!herofand_append_gpu(&runtime->gpus, &gpu)) {
                    free(gpu.hwmon_path);
                    herofand_free_sensor_list(&gpu.sensors);
                    globfree(&temps);
                    globfree(&hwmons);
                    return false;
                }
                memset(&gpu, 0, sizeof(gpu));
            }
        } else {
            for (j = 0; j < temps.gl_pathc; ++j) {
                char label_path[4096];
                char label[128];
                int startup_value;
                size_t temp_len;
                bool keep_sensor = true;

                temp_len = strlen(temps.gl_pathv[j]);
                if (temp_len + 1U >= sizeof(label_path)) {
                    continue;
                }
                memcpy(label_path, temps.gl_pathv[j], temp_len + 1U);
                if (temp_len >= strlen("_input")) {
                    memcpy(label_path + temp_len - strlen("_input"),
                           "_label",
                           strlen("_label") + 1U);
                }

                if (herofand_read_string(label_path, label, sizeof(label))) {
                    if (herofand_label_is_excluded(label)) {
                        keep_sensor = false;
                    }
                    if (keep_sensor
                        && herofand_read_int(temps.gl_pathv[j], &startup_value)
                        && startup_value == 0) {
                        keep_sensor = false;
                    }
                } else {
                    strcpy(label, "(no label)");
                }

                if (keep_sensor
                    && !herofand_append_sensor(&runtime->system_sensors,
                                               temps.gl_pathv[j],
                                               label,
                                               hwmon_name)) {
                    globfree(&temps);
                    globfree(&hwmons);
                    return false;
                }
            }
        }

        globfree(&temps);
    }

    globfree(&hwmons);
    return runtime->system_sensors.count > 0U;
}

static void herofand_log_discovery(const struct herofand_runtime *runtime)
{
    size_t i;
    size_t j;

    fprintf(stdout, "herofand started: NCT=%s\n", runtime->nct_path);
    fprintf(stdout, "Polling %zu non-GPU sensors:\n", runtime->system_sensors.count);
    for (i = 0; i < runtime->system_sensors.count; ++i) {
        fprintf(stdout,
                "  %s [%s / %s]\n",
                runtime->system_sensors.items[i].path,
                runtime->system_sensors.items[i].hwmon_name,
                runtime->system_sensors.items[i].label);
    }

    fprintf(stdout, "Controlling %zu GPU fan controller(s) independently\n", runtime->gpus.count);
    for (i = 0; i < runtime->gpus.count; ++i) {
        fprintf(stdout, "  GPU %zu sensors:\n", i);
        for (j = 0; j < runtime->gpus.items[i].sensors.count; ++j) {
            fprintf(stdout, "    %s\n", runtime->gpus.items[i].sensors.items[j].path);
        }
    }
}

static bool herofand_write_nct_pwm(const struct herofand_runtime *runtime, int channel, int value)
{
    char name[32];
    char *path;
    bool ok;

    if (runtime == NULL) {
        return false;
    }

    snprintf(name, sizeof(name), "pwm%d", channel);
    path = herofand_path_join(runtime->nct_path, name);
    if (path == NULL) {
        return false;
    }

    ok = herofand_write_int(path, value);
    free(path);
    return ok;
}

static bool herofand_write_nct_enable(const struct herofand_runtime *runtime,
                                      int channel,
                                      int value)
{
    char name[32];
    char *path;
    bool ok;

    if (runtime == NULL) {
        return false;
    }

    snprintf(name, sizeof(name), "pwm%d_enable", channel);
    path = herofand_path_join(runtime->nct_path, name);
    if (path == NULL) {
        return false;
    }

    ok = herofand_write_int(path, value);
    free(path);
    return ok;
}

static bool herofand_write_gpu_pwm(const struct herofand_gpu *gpu, int value)
{
    char *path;
    bool ok;

    if (gpu == NULL) {
        return false;
    }

    path = herofand_path_join(gpu->hwmon_path, "pwm1");
    if (path == NULL) {
        return false;
    }

    ok = herofand_write_int(path, value);
    free(path);
    return ok;
}

static bool herofand_write_gpu_enable(const struct herofand_gpu *gpu, int value)
{
    char *path;
    bool ok;

    if (gpu == NULL) {
        return false;
    }

    path = herofand_path_join(gpu->hwmon_path, "pwm1_enable");
    if (path == NULL) {
        return false;
    }

    ok = herofand_write_int(path, value);
    free(path);
    return ok;
}

static bool herofand_set_manual_mode(const struct herofand_runtime *runtime)
{
    size_t i;

    if (!herofand_write_nct_enable(runtime, HEROFAND_CASE_PWM2, 1)
        || !herofand_write_nct_enable(runtime, HEROFAND_CASE_PWM3, 1)
        || !herofand_write_nct_enable(runtime, HEROFAND_CASE_PWM4, 1)
        || !herofand_write_nct_enable(runtime, HEROFAND_PUMP_PWM7, 1)
        || !herofand_write_nct_pwm(runtime, HEROFAND_PUMP_PWM7, 255)) {
        return false;
    }

    for (i = 0; i < runtime->gpus.count; ++i) {
        if (!herofand_write_gpu_enable(&runtime->gpus.items[i], 1)) {
            return false;
        }
    }

    return true;
}

static void herofand_cleanup(const struct herofand_runtime *runtime)
{
    size_t i;

    if (runtime == NULL) {
        return;
    }

    fprintf(stdout, "Restoring fans to full speed...\n");
    (void)herofand_write_nct_pwm(runtime, HEROFAND_CASE_PWM2, 255);
    (void)herofand_write_nct_pwm(runtime, HEROFAND_CASE_PWM3, 255);
    (void)herofand_write_nct_pwm(runtime, HEROFAND_CASE_PWM4, 255);
    (void)herofand_write_nct_pwm(runtime, HEROFAND_PUMP_PWM7, 255);

    for (i = 0; i < runtime->gpus.count; ++i) {
        (void)herofand_write_gpu_enable(&runtime->gpus.items[i], 2);
    }
}

static int herofand_read_sensor_max(const struct herofand_sensor_list *list)
{
    int max = 0;
    size_t i;

    if (list == NULL) {
        return 0;
    }

    for (i = 0; i < list->count; ++i) {
        int value;

        if (!herofand_read_int(list->items[i].path, &value) || value == 0) {
            continue;
        }
        if (value > max) {
            max = value;
        }
    }

    return max;
}

static long herofand_now_seconds(void)
{
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }

    return now.tv_sec;
}

static void herofand_sleep_interval(double seconds)
{
    struct timespec req;

    req.tv_sec = (time_t)seconds;
    req.tv_nsec = (long)((seconds - (double)req.tv_sec) * 1000000000.0);

    while (herofand_running && nanosleep(&req, &req) != 0 && errno == EINTR) {
    }
}

int herofand_run(const struct herofand_runtime_config *config, bool run_once)
{
    struct herofand_runtime runtime;
    struct sigaction action;
    int status = EXIT_FAILURE;

    if (!herofand_validate_config(config)) {
        fprintf(stderr, "invalid runtime config\n");
        return EXIT_FAILURE;
    }

    memset(&runtime, 0, sizeof(runtime));
    memset(&action, 0, sizeof(action));
    herofand_channel_state_init(&runtime.intake_state);
    herofand_channel_state_init(&runtime.exhaust_state);
    herofand_running = 1;

    action.sa_handler = herofand_handle_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) != 0 || sigaction(SIGTERM, &action, NULL) != 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    if (!herofand_discover(&runtime)) {
        herofand_free_runtime(&runtime);
        return EXIT_FAILURE;
    }
    if (!herofand_set_manual_mode(&runtime)) {
        fprintf(stderr, "failed to set manual fan mode\n");
        herofand_cleanup(&runtime);
        herofand_free_runtime(&runtime);
        return EXIT_FAILURE;
    }

    herofand_log_discovery(&runtime);

    while (herofand_running) {
        int max_temp;
        size_t i;
        long now_seconds;
        int applied_tier;

        now_seconds = herofand_now_seconds();
        max_temp = herofand_read_sensor_max(&runtime.system_sensors);

        for (i = 0; i < runtime.gpus.count; ++i) {
            int gpu_max;
            int gpu_tier;

            gpu_max = herofand_read_sensor_max(&runtime.gpus.items[i].sensors);
            if (gpu_max > max_temp) {
                max_temp = gpu_max;
            }

            gpu_tier = herofand_curve_tier(&config->gpu_curve, gpu_max);
            if (herofand_channel_state_apply(&runtime.gpus.items[i].state,
                                             gpu_tier,
                                             now_seconds,
                                             config->downshift_delay_seconds,
                                             &applied_tier)) {
                (void)herofand_write_gpu_pwm(&runtime.gpus.items[i],
                                             herofand_curve_pwm(&config->gpu_curve, applied_tier));
            }
        }

        if (herofand_channel_state_apply(&runtime.intake_state,
                                         herofand_curve_tier(&config->intake_curve, max_temp),
                                         now_seconds,
                                         config->downshift_delay_seconds,
                                         &applied_tier)) {
            int pwm = herofand_curve_pwm(&config->intake_curve, applied_tier);
            (void)herofand_write_nct_pwm(&runtime, HEROFAND_CASE_PWM2, pwm);
            (void)herofand_write_nct_pwm(&runtime, HEROFAND_CASE_PWM3, pwm);
        }

        if (herofand_channel_state_apply(&runtime.exhaust_state,
                                         herofand_curve_tier(&config->exhaust_curve, max_temp),
                                         now_seconds,
                                         config->downshift_delay_seconds,
                                         &applied_tier)) {
            (void)herofand_write_nct_pwm(&runtime,
                                         HEROFAND_CASE_PWM4,
                                         herofand_curve_pwm(&config->exhaust_curve, applied_tier));
        }

        if (run_once) {
            break;
        }

        herofand_sleep_interval(config->interval_seconds);
    }

    herofand_cleanup(&runtime);
    herofand_free_runtime(&runtime);
    status = EXIT_SUCCESS;
    return status;
}
