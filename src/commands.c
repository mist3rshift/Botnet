//
// Created by angaros on 3/31/2025.
//

#include "../include/commands.h"
#include "../include/logging.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h> // For CreateProcess
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include "../include/send_message.h"
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h> // For isdigit

Command *build_command(const char *cmd_id, int delay, const char *program, int expected_code, time_t timestamp, ...) {
    // Allocate memory for the Command structure
    Command *cmd = malloc(sizeof(Command));
    if (!cmd) {
        return NULL;
    }

    // Initialize the Command structure
    strncpy(cmd->cmd_id, cmd_id ? cmd_id : "", sizeof(cmd->cmd_id) - 1); // Copy cmd_id into the fixed-size array
    cmd->cmd_id[sizeof(cmd->cmd_id) - 1] = '\0';                         // Ensure null-termination
    cmd->delay = delay;
    cmd->program = program ? strdup(program) : NULL;
    cmd->expected_exit_code = expected_code;
    cmd->timestamp = timestamp ? timestamp : time(NULL); // Use provided timestamp or set the current time

    // Handle variadic arguments for additional parameters
    va_list args;
    va_start(args, timestamp);

    const char *arg;
    size_t params_len = 0;
    cmd->params = NULL;

    while ((arg = va_arg(args, const char *)) != NULL) {
        cmd->params = realloc(cmd->params, (params_len + 1) * sizeof(char *));
        cmd->params[params_len] = strdup(arg);
        params_len++;
    }
    va_end(args);

    cmd->params = realloc(cmd->params, (params_len + 1) * sizeof(char *));
    cmd->params[params_len] = NULL; // Null-terminate the parameters array

    return cmd;
}

void free_command(Command *cmd) {
    if (!cmd) return;

    // Free the program field if it was dynamically allocated
    if (cmd->program) {
        free(cmd->program);
        cmd->program = NULL;
    }

    // Free the params array if it was dynamically allocated
    if (cmd->params) {
        for (size_t i = 0; cmd->params[i] != NULL; ++i) {
            free(cmd->params[i]);
        }
        free(cmd->params);
        cmd->params = NULL;
    }
}

int send_command(int sockfd, const Command *cmd) {
    if (cmd == NULL) {
        output_log("Command is NULL\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    // Serialize the Command structure
    char buffer[1024];
    output_log("Serializing command...\n", LOG_DEBUG, LOG_TO_CONSOLE);
    serialize_command(cmd, buffer, sizeof(buffer));

    output_log("Serialized command: %s\n", LOG_DEBUG, LOG_TO_CONSOLE, buffer);

    // TODO: FIX "Error sending command: Bad file descriptor" issue here


    // Send the serialized command to the client
    ssize_t bytes_sent = send(sockfd, buffer, strlen(buffer), 0);
    if (bytes_sent < 0) {
        perror("Error sending command");
        output_log("Failed to send command to socket id %d\n", LOG_ERROR, LOG_TO_ALL, sockfd);
        return -1;
    }

    output_log("Successfully sent command to socket id %d\n", LOG_DEBUG, LOG_TO_CONSOLE, sockfd);
    return 0;
}

void receive_command(const int delay, const char *program, ...)
{
    if (delay > 0)
    {
        sleep_ms(delay);
    }
}

// Execute a command from a Command structure, return exit_code or -1
int execute_command(const Command *cmd, char *result_buffer, size_t buffer_size) {
    if (!cmd || !cmd->program) {
        output_log("Invalid command\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // Child process
        close(pipe_fd[0]); // Close read end
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to pipe
        dup2(pipe_fd[1], STDERR_FILENO); // Redirect stderr to pipe
        close(pipe_fd[1]);

        // Prepare arguments for execvp
        char *args[64]; // Adjust size as needed
        size_t arg_index = 0;

        args[arg_index++] = cmd->program; // First argument is the program name

        if (cmd->params) {
            for (size_t i = 0; cmd->params[i] != NULL && arg_index < (sizeof(args) / sizeof(args[0])) - 1; ++i) {
                args[arg_index++] = cmd->params[i];
            }
        }

        args[arg_index] = NULL; // Null-terminate the argument list

        // Execute the command
        execvp(cmd->program, args);
        perror("execvp"); // If execvp fails, write the error to stderr
        exit(1);
    } else {
        // Parent process
        close(pipe_fd[1]); // Close write end

        memset(result_buffer, 0, buffer_size); // Clear the buffer before reading

        // Read output from the pipe
        ssize_t bytes_read = 0;
        size_t total_bytes = 0;
        while ((bytes_read = read(pipe_fd[0], result_buffer + total_bytes, buffer_size - total_bytes - 1)) > 0) {
            total_bytes += bytes_read;
            if (total_bytes >= buffer_size - 1) {
                break; // Stop reading if the buffer is full
            }
        }

        if (total_bytes > 0) {
            result_buffer[total_bytes] = '\0'; // Null-terminate the result
        } else {
            strcpy(result_buffer, "Error reading command output");
        }

        close(pipe_fd[0]);
        waitpid(pid, NULL, 0); // Wait for the child process to finish
    }

    return 0;
}

void serialize_command(const Command *cmd, char *buffer, size_t buffer_size) {
    if (!cmd || !buffer) return;

    // Serialize the params into a single space-separated string
    char params_buffer[512] = {0};
    if (cmd->params) {
        for (size_t i = 0; cmd->params[i] != NULL; ++i) {
            if (i > 0) strncat(params_buffer, " ", sizeof(params_buffer) - strlen(params_buffer) - 1);
            strncat(params_buffer, cmd->params[i], sizeof(params_buffer) - strlen(params_buffer) - 1);
        }
    }

    // Serialize the Command structure, including the timestamp
    snprintf(buffer, buffer_size, "%d|%s|%d|%s|%d|%ld|%s", 
             cmd->order_type, 
             cmd->cmd_id[0] ? cmd->cmd_id : "0", // Include cmd_id, even if empty
             cmd->delay, 
             cmd->program, 
             cmd->expected_exit_code, 
             cmd->timestamp, // Serialize the timestamp
             params_buffer);
}

// Helper function to check if a string is all digits (optionally allow negative sign)
static int is_number(const char *s) {
    if (!s || !*s) return 0;
    if (*s == '-') ++s;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        ++s;
    }
    return 1;
}

void deserialize_command(char *buffer, Command *cmd) {
    if (!cmd || !buffer) return;

    // Allocate memory for the Command structure
    memset(cmd, 0, sizeof(Command));
    cmd->order_type = UNKNOWN; // Default to UNKNOWN

    output_log("Got deserialization defaults\n", LOG_DEBUG, LOG_TO_CONSOLE);

    // Deserialize the Command structure
    char *saveptr = NULL;
    char *token = strtok_r(buffer, "|", &saveptr);

    // Order type
    if (!token) return;
    output_log("token -> %s\n", LOG_DEBUG, LOG_TO_CONSOLE, token);

    if (!is_number(token)) {
        output_log("Order type is not a number, marking as UNKNOWN\n", LOG_DEBUG, LOG_TO_CONSOLE);
        cmd->order_type = UNKNOWN;
        return;
    }

    int order_type = atoi(token);
    if (order_type < 0 || order_type > UNKNOWN) {
        cmd->order_type = UNKNOWN;
        return;
    }

    cmd->order_type = order_type;
    output_log("Got order type -> %d\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd->order_type);

    // Command ID
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) return;
    strncpy(cmd->cmd_id, token, sizeof(cmd->cmd_id) - 1);
    cmd->cmd_id[sizeof(cmd->cmd_id) - 1] = '\0';

    output_log("Got command ID -> %s\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd->cmd_id);

    // Delay
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) return;
    cmd->delay = atoi(token);

    output_log("Got delay -> %d\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd->delay);

    // Program
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) return;
    cmd->program = strdup(token);

    output_log("Got program -> %s\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd->program);

    // Expected exit code
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) return;
    cmd->expected_exit_code = atoi(token);

    output_log("Got expected exit -> %d\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd->expected_exit_code);

    // Timestamp
    token = strtok_r(NULL, "|", &saveptr);
    if (!token) return;
    cmd->timestamp = atol(token);

    output_log("Got timestamp -> %lld\n", LOG_DEBUG, LOG_TO_CONSOLE, (long long) cmd->timestamp);

    // Params
    if (saveptr && strlen(saveptr) > 0) {
        size_t params_count = 0;
        char *param_token = NULL;
        char *param_saveptr = NULL;

        // Split params by spaces
        param_token = strtok_r(saveptr, " ", &param_saveptr);

        // Count and allocate params
        while (param_token) {
            cmd->params = realloc(cmd->params, (params_count + 1) * sizeof(char *));
            cmd->params[params_count] = strdup(param_token);
            params_count++;
            param_token = strtok_r(NULL, " ", &param_saveptr);
        }

        // Null-terminate the params array
        cmd->params = realloc(cmd->params, (params_count + 1) * sizeof(char *));
        cmd->params[params_count] = NULL;
    } else {
        cmd->params = NULL;
    }

    output_log("Deserialization end\n", LOG_DEBUG, LOG_TO_CONSOLE);
}

//construit un tableau de 7 Command 
Command **commands_sysinfo(void){
    int num_cmds = 7;
    Command **cmds = malloc(num_cmds * sizeof(Command*));
    if (cmds == NULL) {
        return NULL;
    }

    cmds[0] = build_command("S_uname", 0, "uname", 0, time(NULL), "-a");
    cmds[1] = build_command("S_uptime", 0, "uptime", 0, time(NULL), "a");
    cmds[2] = build_command("S_ipa", 0, "ip", 0, time(NULL), "a");
    cmds[3] = build_command("S_ipr", 0, "ip", 0, time(NULL), "r");
    cmds[4] = build_command("S_iptables", 0, "iptables", 0, time(NULL), "-L");
    cmds[5] = build_command("S_cpu", 0, "lscpu", 0, time(NULL));
    cmds[6] = build_command("S_procs", 0, "ps", 0, time(NULL), "au");
    
    for (int i = 0; i<num_cmds; i++){
        cmds[i]->order_type = COMMAND_;
    }

    return cmds;
}

 //libère un tableau de commandes
void free_commands(Command **cmds, int num_cmds){
    for (int i = 0; i < num_cmds; i++){
        free(cmds[i]);
    }
    free(cmds);
}
