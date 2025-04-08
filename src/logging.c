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

bool suppress_errors_flag = false;

enum LogLevel current_log_level = LOG_LEVEL_INFO; // Default log level

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
    // Map LogCode to LogLevel
    enum LogLevel log_level;
    switch (code) {
        case LOG_ERROR:
            log_level = LOG_LEVEL_ERROR;
            break;
        case LOG_WARNING:
            log_level = LOG_LEVEL_WARNING;
            break;
        case LOG_INFO:
            log_level = LOG_LEVEL_INFO;
            break;
        case LOG_DEBUG:
            log_level = LOG_LEVEL_DEBUG;
            break;
        default:
            log_level = LOG_LEVEL_INFO;
            break;
    }

    // Skip logs below the current log level
    if ((log_level > current_log_level) || (log_level == LOG_LEVEL_ERROR && suppress_errors_flag)) {
        return;
    }

    FILE *file = NULL;
    if (type == LOG_TO_ALL || type == LOG_TO_FILE) {
        file = fopen("main.log", "a+");
        if (file == NULL) {
            fprintf(stderr, "\033[1;31m[ERR]\033[0m - Could not open log file.\n");
            return;
        }
    }

    va_list args;
    va_start(args, type);

    // Check for NULL format string
    if (fmt == NULL) {
        fprintf(stderr, "\033[1;31m[ERR]\033[0m - Format string is NULL.\n");
        va_end(args);
        if (file) fclose(file);
        return;
    }

    // Create the log portion
    char timestamp[30];
    get_timestamp(timestamp, sizeof(timestamp));

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
        fprintf(file, "[%s] [%s] - ", timestamp, error_type);
        vfprintf(file, fmt, args);
        fclose(file);
    }

    // Log to console
    if (type == LOG_TO_ALL || type == LOG_TO_CONSOLE) {
        va_end(args); // End the current va_list
        va_start(args, type); // Reinitialize va_list
        if (code == LOG_ERROR) {
            fprintf(stderr, "%s[%s] [%s]\033[0m - ", color_code, timestamp, error_type);
            vfprintf(stderr, fmt, args);
        } else {
            fprintf(stdout, "%s[%s] [%s]\033[0m - ", color_code, timestamp, error_type);
            vfprintf(stdout, fmt, args);
        }
    }

    va_end(args);
}