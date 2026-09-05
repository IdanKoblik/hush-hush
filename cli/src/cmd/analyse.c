#include "../args.h"
#include "command.h"

#include <core/analysis/stats.h>
#include <core/fs/file.h>
#include <core/log.h>

#include <flag.h>
#include <stdbool.h>
#include <stdio.h>

#define NAME_WIDTH 26

static void print_row(enum StatMethod method, const struct StatResult *result) {
    printf("  %-*s", NAME_WIDTH, stat_method_name(method));

    if (!result->applicable) {
        printf("%12s\n", "n/a");
        return;
    }

    printf("%10.2f %%%10.0f %%%12zu %s\n", result->percent, result->reference, result->population, stat_method_unit(method));
}

static int exec(int argc, char *argv[]) {
    const char *target = shift_args(&argc, &argv);
    if (!target)
        return EXEC_USAGE_ERROR;

    bool explain = false;

    argc++;
    argv--;

    flag_bool_var(&explain, "e", false, "Explain what each method measures");

    if (!flag_parse(argc, argv)) {
        flag_print_error(stderr);
        return EXEC_USAGE_ERROR;
    }

    const enum FileType type = get_file_type(target);
    if (type == TYPE_NOT_FOUND) {
        ERROR("Target file was not found");
        return EXEC_GENERIC_ERROR;
    }

    if (!is_image_file(type)) {
        ERROR("%s is not a PNG or a JPEG", target);
        return EXEC_GENERIC_ERROR;
    }

    struct StatSuite suite;
    if (stats_analyse(target, &suite) != 0) {
        ERROR("Failed to analyse %s", target);
        return EXEC_GENERIC_ERROR;
    }

    INFO("Target: %s", target);

    if (suite.coefficients > 0)
        INFO("Type: %s, %d x %d, %d channels, %zu usable coefficients", file_type_name(suite.type), suite.width, suite.height, suite.channels, suite.coefficients);
    else
        INFO("Type: %s, %d x %d, %d channels", file_type_name(suite.type), suite.width, suite.height, suite.channels);

    printf("\n  %-*s%12s%12s%12s\n", NAME_WIDTH, "Method", "Measured", "Random bits", "Population");

    for (size_t i = 0; i < STAT_METHOD_COUNT; i++)
        print_row((enum StatMethod)i, &suite.results[i]);

    if (!explain)
        return EXEC_OK;

    printf("\n");
    for (size_t i = 0; i < STAT_METHOD_COUNT; i++)
        printf("  %s\n    %s\n\n", stat_method_name((enum StatMethod)i), stat_method_summary((enum StatMethod)i));

    return EXEC_OK;
}

static const struct Command analyse_cmd = {
    .name = "analyse",
    .description = "Score how much a carrier looks like it is hiding something.",
    .usage = "Usage: analyse <target_file> [-e]\n",
    .exec = exec,
};

COMMAND(analyse_cmd);
