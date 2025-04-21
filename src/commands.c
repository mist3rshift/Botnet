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

void free_command(Command *cmd)
{
    if (!cmd)
        return;
    if (cmd->program)
        free((char *)cmd->program);
    if (cmd->params)
    {
        for (int i = 0; cmd->params[i] != NULL; i++)
        {
            free(cmd->params[i]);
        }
        free(cmd->params);
    }
    free(cmd);
}


int send_command(int sockfd, const Command *cmd) {
    if (cmd == NULL) {
        output_log("Command is NULL\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    // Serialize the Command structure
    char buffer[1024];
    serialize_command(cmd, buffer, sizeof(buffer));

    // Send the serialized command to the client
    ssize_t bytes_sent = send(sockfd, buffer, strlen(buffer), 0);
    if (bytes_sent < 0) {
        perror("Error sending command");
        output_log("Failed to send command to socket id %d\n", LOG_ERROR, LOG_TO_ALL, sockfd);
        return -1;
    }

    output_log("Command sent successfully: %s\n", LOG_INFO, LOG_TO_ALL, buffer);
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

    char command[1024] = {0};

    // Build the command string
    snprintf(command, sizeof(command), "%s", cmd->program);
    if (cmd->params) {
        for (size_t i = 0; cmd->params[i] != NULL; ++i) {
            strncat(command, " ", sizeof(command) - strlen(command) - 1);
            strncat(command, cmd->params[i], sizeof(command) - strlen(command) - 1);
        }
    }

    output_log("Executing command: %s\n", LOG_INFO, LOG_TO_ALL, command);

    // Open a pipe to capture the output of the command
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        output_log("Failed to execute command: %s\n", LOG_ERROR, LOG_TO_ALL, strerror(errno));
        return -1;
    }

    // Read the output of the command
    size_t total_read = 0;
    while (fgets(result_buffer + total_read, buffer_size - total_read - 1, pipe)) {
        total_read = strlen(result_buffer);
        if (total_read >= buffer_size - 1) {
            output_log("Command output truncated due to buffer size\n", LOG_WARNING, LOG_TO_ALL);
            break;
        }
    }

    // Close the pipe and get the exit code
    int exit_code = pclose(pipe);
    if (exit_code == -1) {
        output_log("Failed to close command pipe: %s\n", LOG_ERROR, LOG_TO_ALL, strerror(errno));
        return -1;
    }

    output_log("Command executed with exit code: %d\n", LOG_INFO, LOG_TO_ALL, exit_code);
    return exit_code;
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

void deserialize_command(char *buffer, Command *cmd) {
    if (!cmd || !buffer) return;

    // Allocate memory for the Command structure
    memset(cmd, 0, sizeof(Command));

    // Deserialize the Command structure
    char *saveptr = NULL;
    char *token = strtok_r(buffer, "|", &saveptr);

    // Order type
    if (token) {
        cmd->order_type = atoi(token);
    }

    // Command ID
    token = strtok_r(NULL, "|", &saveptr);
    if (token) {
        strncpy(cmd->cmd_id, token, sizeof(cmd->cmd_id) - 1);
        cmd->cmd_id[sizeof(cmd->cmd_id) - 1] = '\0';
    }

    // Delay
    token = strtok_r(NULL, "|", &saveptr);
    if (token) {
        cmd->delay = atoi(token);
    }

    // Program
    token = strtok_r(NULL, "|", &saveptr);
    if (token) {
        cmd->program = strdup(token);
    }

    // Expected exit code
    token = strtok_r(NULL, "|", &saveptr);
    if (token) {
        cmd->expected_exit_code = atoi(token);
    }

    // Timestamp
    token = strtok_r(NULL, "|", &saveptr);
    if (token) {
        cmd->timestamp = atol(token);
    }

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
}
