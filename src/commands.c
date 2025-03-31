//
// Created by angaros on 3/31/2025.
//

#include "../include/commands.h"
#include "../include/logging.h"

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>  // For CreateProcess
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

Command* build_command(const char* cmd_id, int delay, const char* program, int expected_code, ...) {
    Command* cmd = malloc(sizeof(Command));
    if (!cmd) return NULL;

    strncpy(cmd->cmd_id, cmd_id, sizeof(cmd->cmd_id)-1);
    cmd->delay = delay;
    cmd->program = strdup(program);
    cmd->expected_exit_code = expected_code;
    cmd->timestamp = time(NULL);

    // Variadic fct, start arglist
    va_list args;
    va_start(args, expected_code);

    // How many args?
    int param_count = 0;
    const char* arg;
    while ((arg = va_arg(args, const char*)) != NULL) {
        param_count++;
    }
    va_end(args);

    // Prepare cmd params
    cmd->params = malloc((param_count + 1) * sizeof(char*));
    if (!cmd->params) {
        free(cmd);
        return NULL;
    }

    va_start(args, expected_code);
    for (int i = 0; i < param_count; i++) {
        cmd->params[i] = strdup(va_arg(args, const char*));
    }
    cmd->params[param_count] = NULL; // Add \x00 null term
    va_end(args);

    // TODO: Signature??

    return cmd;
}

void send_command(const int delay, const char *program, ...) {
    if (delay > 0) {
        sleep_ms(delay);
    }
}
void receive_command(const int delay, const char *program, ...) {
    if (delay > 0) {
        sleep_ms(delay);
    }
}

int execute_command(const Command* cmd) {
    if (cmd->delay > 0) {
        sleep_ms(cmd->delay);
    }

#ifdef _WIN32
    // Windows-specific implementation
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    char cmdline[1024];

    // Build command line (program + params)
    snprintf(cmdline, sizeof(cmdline), "\"%s\"", cmd->program); // Quote the program name
    for (char** p = cmd->params; *p; p++) {
        strncat(cmdline, " ", sizeof(cmdline) - strlen(cmdline) - 1);
        strncat(cmdline, *p, sizeof(cmdline) - strlen(cmdline) - 1);
    }

    // Attempt to create the process
    if (!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        DWORD error_code = GetLastError();
        //output_log("CreateProcess failed with error code %lu", LOG_ERROR, LOG_TO_CONSOLE, error_code);
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)exit_code;

#else
    // Unix implementation
    pid_t pid = fork();
    if (pid == 0) {
        execvp(cmd->program, cmd->params);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}
