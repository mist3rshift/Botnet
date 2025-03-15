//
// Created by angaros on 3/10/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "../include/logging.h"


void get_timestamp(char *buffer, size_t bufsize) {
    struct timeval tv;
    struct tm *tm_info;

#ifdef _WIN32 // On Windows
    SYSTEMTIME st;
    GetLocalTime(&st); // Get the current time w/ 3-digit precision on milliseconds
    snprintf(buffer, bufsize, "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else // Other OS
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec); // Get current time
    snprintf(buffer, bufsize, "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             (int)(tv.tv_usec / 1000)); // Convert microseconds to milliseconds
#endif
}

void output_log(const char *fmt, const enum LogCode code, const enum LogType type, ...) {
    // Print logs to file, console, or both with error type and color coding.
    FILE *file = NULL;
    if (type == LOG_TO_ALL || type == LOG_TO_FILE) { // Only open when required, and also prevent closing unopened file!
        file = fopen("main.log", "a+"); // Create (file not exists) / append to log  (file exists)
        if (file == NULL) { // Error somewhere
            fprintf(stderr, "\033[1;31m[ERR]\033[0m - Could not open log file.\n");
            return;
        }
    }

    va_list args; // Init the argument list
    va_start(args, type); // Setup arg list following function

    // Create the log portion
    char timestamp[30]; // Buffer for timestamp
    get_timestamp(timestamp, sizeof(timestamp));

    // Determine error "name" to use in log&console, and color to use
    const char *error_type;
    const char *color_code;
    switch (code) {
        case LOG_INFO:
            error_type = "INF";
            color_code = "\033[1;34m";
            break;
        case LOG_WARNING:
            error_type = "WRN";
            color_code = "\033[1;33m";
            break;
        case LOG_ERROR:
            error_type = "ERR";
            color_code = "\033[1;31m";
            break;
        case LOG_DEBUG:
            error_type = "DBG";
            color_code = "\033[0m";
            break;
        default:
            error_type = "UKWN";
            color_code = "\033[0m";
            break;
    }

    // Log to file
    if (type == LOG_TO_ALL || type == LOG_TO_FILE) {
        fprintf(file, "[%s] [%s] - ", timestamp, error_type); // Timestamp and error type
        vfprintf(file, fmt, args); // Log message
        fclose(file); // Close the file
    }

    // Log to console
    if (type == LOG_TO_ALL || type == LOG_TO_CONSOLE) {
        if (code == LOG_ERROR) {
          // Specify Standard Error output when the log is an error
            fprintf(stderr, "%s[%s] [%s]\033[0m - ", color_code, timestamp, error_type); // to console (only default formatting), with color at start
            vfprintf(stderr, fmt, args); // To console, rest of args (content of the string)
        } else {
            fprintf(stdout, "%s[%s] [%s]\033[0m - ", color_code, timestamp, error_type); // to console (only default formatting), with color at start
            vfprintf(stdout, fmt, args); // To console, rest of args (content of the string)
        }
    }

    va_end(args); // Remove args call
}