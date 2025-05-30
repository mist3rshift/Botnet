#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#include <time.h>
enum OrderType {
    COMMAND_ = 0,
    ASKLOGS_ = 1,
    ASKSTATE = 2,
    DDOSATCK = 3,
    FLOODING = 4,
    DOWNLOAD = 5,
    UPDATE = 6,
    SYSINFO = 7,
    UNKNOWN = 99
};

typedef struct Command {
    char cmd_id[64];          // Fixed-size array for cmd_id
    int delay;                // Optional delay in milliseconds
    char *program;            // Base program (e.g., "ls", "cat")
    int expected_exit_code;   // Optional expected exit code
    char **params;            // Null-terminated array of parameters
    enum OrderType order_type; // Type of command
    time_t timestamp;         // Timestamp for when the command was created
} Command;


static inline void sleep_ms(unsigned milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

Command *build_command(const char *cmd_id, int delay, const char *program, int expected_code, time_t timestamp, ...);
void free_command(Command *cmd);
int execute_command(const Command *cmd, char *result_buffer, size_t buffer_size);
static int is_number(const char *s);
void serialize_command(const Command* cmd, char* buffer, size_t buffer_size);
void deserialize_command(char *buffer, Command *cmd);
Command *commands_sysinfo(void);
void free_commands(Command **cmds, int num_cmds);
#endif
