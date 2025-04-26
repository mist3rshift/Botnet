#include "../include/logging.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "../include/client/client_constants.h"

// Global variable to store the server address
char server_address[INET_ADDRSTRLEN] = DEFAULT_SERVER_ADDR;
char server_port[6] = DEFAULT_SERVER_PORT; // Default port is stored as a string

void parse_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-vv") == 0) {
            current_log_level = LOG_LEVEL_DEBUG; // Show all debug logs and above
        } else if (strcmp(argv[i], "--info") == 0 || strcmp(argv[i], "-v") == 0) {
            current_log_level = LOG_LEVEL_INFO; // Show all info logs and above (default)
        } else if (strcmp(argv[i], "--error") == 0) {
            current_log_level = LOG_LEVEL_ERROR; // Only show errors
        } else if (strcmp(argv[i], "--suppress-errors") == 0) {
            suppress_errors_flag = true; // Enable error suppression (no errors in console/file)
        } else if ((strcmp(argv[i], "--addr") == 0 || strcmp(argv[i], "-a") == 0) && i + 1 < argc) {
            struct in_addr addr;
            if (inet_pton(AF_INET, argv[i + 1], &addr) == 1) {
                strncpy(server_address, argv[i + 1], INET_ADDRSTRLEN - 1);
                server_address[INET_ADDRSTRLEN - 1] = '\0';
                i++; // DO NOT REMOVE - Skips the Addr passed after --addr
            } else {
                fprintf(stderr, "Invalid IP address provided for --addr: %s\n", argv[i + 1]);
                exit(EXIT_FAILURE);
            }
        } else if ((strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) && i + 1 < argc) {
            int port = atoi(argv[i + 1]);
            if (port > 0 && port <= 65535) {
                strncpy(server_port, argv[i + 1], 6);
                server_port[5] = '\0';
                i++; // DO NOT REMOVE - Skips the port passed after --port
            } else {
                fprintf(stderr, "Invalid port provided for --port: %s\n", argv[i + 1]);
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
}