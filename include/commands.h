#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

#include <time.h>

typedef struct {
    char cmd_id[32]; // Understand which command is being sent by ID
    int delay; // Delay to wait for cmd execution
    const char *program; // Program name (ls, cat, ...)
    char **params; // Params to the command
    int expected_exit_code; // Expect a return code
    //unsigned char sig[64]; Should we sign??
    time_t timestamp; // For logs, record creation date
} Command;

static inline void sleep_ms(unsigned milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

Command* build_command(const char* cmd_id, int delay, const char* program, int expected_code, ...);
void send_command(const int delay, const char *program, ...);
void receive_command(const int delay, const char *program, ...);
int execute_command(const Command* cmd);

#endif