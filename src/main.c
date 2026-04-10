#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "controller.h"

#ifndef HEROFAND_VERSION
#define HEROFAND_VERSION "dev"
#endif

static const struct option herofand_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"once", no_argument, NULL, 'o'},
    {"verbose", no_argument, NULL, 'v'},
    {"version", no_argument, NULL, 'V'},
    {NULL, 0, NULL, 0},
};

static void herofand_print_usage(const char *argv0) {
    fprintf(stdout,
            "Usage: %s [--once] [--verbose]\n"
            "       %s --version\n",
            argv0, argv0);
}

int main(int argc, char **argv) {
    struct herofand_runtime_config config;
    bool run_once = false;
    int option;

    config = herofand_default_config();

    while ((option = getopt_long(argc, argv, "hovV", herofand_options, NULL)) != -1) {
        switch (option) {
        case 'h':
            herofand_print_usage(argv[0]);
            return EXIT_SUCCESS;
        case 'o':
            run_once = true;
            break;
        case 'v':
            config.verbose_logging = true;
            break;
        case 'V':
            fprintf(stdout, "%s\n", HEROFAND_VERSION);
            return EXIT_SUCCESS;
        default:
            herofand_print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!herofand_validate_config(&config)) {
        fprintf(stderr, "invalid default config\n");
        return EXIT_FAILURE;
    }

    if (config.verbose_logging) {
        fprintf(stdout, "herofand %s starting (interval=%.1fs, downshift=%ds)\n", HEROFAND_VERSION,
                config.interval_seconds, config.downshift_delay_seconds);
    }

    return herofand_run(&config, run_once);
}
