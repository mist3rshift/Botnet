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

Command *build_command(const char *cmd_id, int delay, const char *program, int expected_code, ...)
{
    Command *cmd = malloc(sizeof(Command));
    if (!cmd)
        return NULL;

    strncpy(cmd->cmd_id, cmd_id, sizeof(cmd->cmd_id) - 1);
    cmd->delay = delay;
    cmd->program = strdup(program);
    cmd->expected_exit_code = expected_code;
    cmd->timestamp = time(NULL);

    // Variadic fct, start arglist
    va_list args;
    va_start(args, expected_code);

    // How many args?
    int param_count = 0;
    const char *arg;
    while ((arg = va_arg(args, const char *)) != NULL)
    {
        param_count++;
    }
    va_end(args);

    // Prepare cmd params
    cmd->params = malloc((param_count + 1) * sizeof(char *));
    if (!cmd->params)
    {
        free(cmd);
        return NULL;
    }

    va_start(args, expected_code);
    for (int i = 0; i < param_count; i++)
    {
        cmd->params[i] = strdup(va_arg(args, const char *));
    }
    cmd->params[param_count] = NULL; // Add \x00 null term
    va_end(args);

    // TODO: Signature??

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

void send_command(const int delay, const char *program, ...)
{
    if (delay > 0)
    {
        sleep_ms(delay);
    }
}
void receive_command(const int delay, const char *program, ...)
{
    if (delay > 0)
    {
        sleep_ms(delay);
    }
}

int execute_command(const Command *cmd)
{
    if (!cmd || !cmd->program || !cmd->params)
    {
        fprintf(stderr, "Invalid command structure\n");
        return -1;
    }
    if (cmd->delay > 0)
    {
        sleep_ms(cmd->delay);
    }
    pid_t pid = fork(); // duplicates the process
    if (pid < 0)
    {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) // Child process is executing
    {
        execvp(cmd->program, cmd->params);
    }
    else // Parent process is executing : wait for the end of child process
    {
        int status;
        if (waitpid(pid, &status, 0) == -1)
        {
            perror("waitpid failed");
            return -1;
        }
        if (WIFEXITED(status))
        {
            int exit_code = WEXITSTATUS(status);
            return (exit_code == cmd->expected_exit_code) ? 0 : exit_code;
        }
        else
        {
            printf("Error while executing command\n");
            return -1;
        }
    }
}
