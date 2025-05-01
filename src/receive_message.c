#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../include/server/hash_table.h"
#include "../include/logging.h"
#include "../include/server/main.h"
#include "../include/server/server_utils.h"
#include "../include/server/server_constants.h"

extern ClientHashTable hash_table;

int initialize_client_log(const char *client_id) {
    if (!client_id) {
        output_log("Error: Client ID is NULL\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    // Ensure the client messages directory exists
    struct stat st = {0};
    if (stat(CLIENT_MESSAGES_DIR, &st) == -1 && mkdir(CLIENT_MESSAGES_DIR, 0755) != 0) {
        output_log("Error creating directory for client messages\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    // Create the client log file
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s_messages.txt", CLIENT_MESSAGES_DIR, client_id);
    FILE *file = fopen(filepath, "a");
    if (file) {
        fclose(file);
        output_log("Initialized log file for client %s\n", LOG_INFO, LOG_TO_ALL, client_id);
    } else {
        output_log("Error initializing log file for client %s\n", LOG_ERROR, LOG_TO_ALL, client_id);
        return -1;
    }

    return 0;
}

int write_to_client_log(Client *client, char *message) {
    if (!client) {
        output_log("Error: passed client is NULL\n", LOG_ERROR, LOG_TO_ALL);
        return -1; // Indicate failure
    }

    output_log("OK - Got client struct from ID\n", LOG_DEBUG, LOG_TO_CONSOLE);

    // Create the directory if it doesn't exist
    struct stat st = {0};
    if (stat(CLIENT_MESSAGES_DIR, &st) == -1 && mkdir(CLIENT_MESSAGES_DIR, 0755) != 0) {
        output_log("Error creating directory for client messages\n", LOG_ERROR, LOG_TO_ALL);
        return -1; // Indicate failure
    }

    // Construct the file path
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s_messages.txt", CLIENT_MESSAGES_DIR, client->id);
    output_log("Writing to client log\n", LOG_DEBUG, LOG_TO_CONSOLE);
    FILE *file = fopen(filepath, "a");
    if (file) {
        fprintf(file, "%s:%s\n", client->id, message);
        fclose(file);
        output_log("Message from client %s on socket %d written to file\n", LOG_INFO, LOG_TO_ALL, client->id, client->socket);
    } else {
        output_log("Error writing message from %s to file\n", LOG_ERROR, LOG_TO_ALL, client->id);
        return -1; // Indicate failure
    }

    output_log("OK - Wrote to client log file\n", LOG_DEBUG, LOG_TO_CONSOLE);

    return 0;
}

int receive_message_server(
    int client_socket,
    char *(*generate_client_id)(int),
    Client *(*find_client_func)(ClientHashTable *, const char *),
    ssize_t (*recv_func)(int, void *, size_t, int)
) {
    char buffer[1024];
    struct timeval timeout = {5, 0}; // 5-second timeout
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t bytes_received = recv_func(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            output_log("Client disconnected: socket %d\n", LOG_INFO, LOG_TO_ALL, client_socket);
        } else {
            output_log("Error receiving message from client socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        }
        return -1;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received message

    // Generate client ID
    char *client_id = generate_client_id(client_socket);
    if (!client_id) {
        output_log("Error: Could not generate client ID for socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        return -1;
    }

    // Find the client in the hash table
    Client *client = find_client_func(&hash_table, client_id);
    if (!client) {
        output_log("Error: Client with ID %s not found\n", LOG_ERROR, LOG_TO_ALL, client_id);
        free(client_id);
        return -1;
    }

    // Write the message to the client's log file
    int result = write_to_client_log(client, buffer);
    free(client_id);
    return result;
}

int receive_message_client(int sockfd, char *buffer, size_t buffer_size) {
    ssize_t bytes_received = recv(sockfd, buffer, buffer_size - 1, 0);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            output_log("Server disconnected\n", LOG_ERROR, LOG_TO_ALL);
        } else {
            perror("Error receiving message");
            output_log("Error receiving message from server\n", LOG_ERROR, LOG_TO_ALL);
        }
        return -1; // Indicate failure
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received message
    output_log("Raw message received: %s\n", LOG_DEBUG, LOG_TO_CONSOLE, buffer);

    return 0; // Indicate success
}