#include "../include/logging.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "../include/client/client_constants.h"
#include "../include/server/server_constants.h"

// Global variable to store the server address
char server_address[INET_ADDRSTRLEN];
char server_port[6]; // Default port is stored as a string
bool enable_cli = false; // Get global var enable_cli and set to true (default)

void init_launch_arguments_defaults() {
    strncpy(server_address, DEFAULT_SERVER_ADDR, INET_ADDRSTRLEN - 1);
    server_address[INET_ADDRSTRLEN - 1] = '\0';
    strncpy(server_port, DEFAULT_SERVER_PORT, 5);
    server_port[5] = '\0';
    enable_cli = false;
}

void print_help() {
    printf("Usage: program [OPTIONS]\n");
    printf("Options:\n");
    printf("  -h, --help               Show this help message and exit.\n");
    printf("  -vv, --debug             Enable debug logs (show all debug logs and above).\n");
    printf("  -v, --info               Enable info logs (show all info logs and above).\n");
    printf("  -e, --error              Enable error logs (only show errors, default).\n");
    printf("  -se, --suppress-errors   Suppress error logs (no errors in console or file).\n");
    printf("  -a, --addr <IP>          Specify the server IP address (client only).\n");
    printf("  -p, --port <PORT>        Specify the server port (client only).\n");
    printf("  -c, --cli                Enable CLI mode (server only).\n");
    exit(EXIT_SUCCESS);
}

void parse_arguments(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            // Print help and exit
            print_help();
        } else if (strcmp(argv[i], "--debug") == 0 || strcmp(argv[i], "-vv") == 0) {
            // SERVER / CLIENT - Specify Debug logs
            current_log_level = LOG_LEVEL_DEBUG; // Show all debug logs and above
        } else if (strcmp(argv[i], "--info") == 0 || strcmp(argv[i], "-v") == 0) {
            // SERVER / CLIENT - Specify Info logs
            current_log_level = LOG_LEVEL_INFO; // Show all info logs and above
        } else if (strcmp(argv[i], "--error" ) == 0 || strcmp(argv[i], "-e") == 0) {
            // SERVER / CLIENT - Specify Error logs
            current_log_level = LOG_LEVEL_ERROR; // Only show errors (default)
        } else if (strcmp(argv[i], "--suppress-errors") == 0 || strcmp(argv[i], "-se") == 0) {
            // SERVER / CLIENT - Remove errors from logs
            suppress_errors_flag = true; // Enable error suppression (no errors in console/file)
        } else if ((strcmp(argv[i], "--addr") == 0 || strcmp(argv[i], "-a") == 0) && i + 1 < argc) {
            // CLIENT - Specify server IP
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
            // CLIENT - Specify server port
            int port = atoi(argv[i + 1]);
            if (port > 0 && port <= 65535) {
                strncpy(server_port, argv[i + 1], 6);
                server_port[5] = '\0';
                i++; // DO NOT REMOVE - Skips the port passed after --port
            } else {
                fprintf(stderr, "Invalid port provided for --port: %s\n", argv[i + 1]);
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "--cli") == 0 || strcmp(argv[i], "-c") == 0) {
            // SERVER - Do not open console
            enable_cli = true;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    if (enable_cli == true) {
        // When CLI activated, we want to output nothing in the console.
        current_log_level = LOG_LEVEL_ERROR;
        suppress_errors_flag = true; 
    }
}