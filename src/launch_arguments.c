#include "../include/logging.h"

#include <string.h>

void parse_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-vv") == 0) {
            current_log_level = LOG_LEVEL_DEBUG;
        } else if (strcmp(argv[i], "--info") == 0 || strcmp(argv[i], "-v") == 0) {
            current_log_level = LOG_LEVEL_INFO;
        } else if (strcmp(argv[i], "--error") == 0) {
            current_log_level = LOG_LEVEL_ERROR;
        } else if (strcmp(argv[i], "--suppress-errors") == 0) {
            suppress_errors_flag = true; // Enable error suppression
        }
    }
}