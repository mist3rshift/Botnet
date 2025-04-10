#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "../include/server/main.h"
#include "../include/server/hash_table.h" // Access hash table
#include "../include/logging.h" // logging

extern ClientHashTable hash_table;

void receive_message_server(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        output_log("Error receiving message from client", LOG_ERROR, LOG_TO_ALL);
        return;
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received message

    // Find the client in the hash table using the socket
    char *client_id = generate_client_id_from_socket(client_socket); // Assume this function exists
    if (client_id == NULL) {
        output_log("Error: Could not generate client ID for socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        return;
    }

    Client *client = find_client(&hash_table, client_id);
    if (client == NULL) {
        output_log("Error: Client with ID %s not found\n", LOG_ERROR, LOG_TO_ALL, client_id);
        free(client_id);
        return;
    }

    // Define the directory for client message files
    const char *directory = "client_messages";

    // Create the directory if it doesn't exist
    struct stat st = {0};
    if (stat(directory, &st) == -1) {
        if (mkdir(directory, 0755) != 0) {
            output_log("Error creating directory for client messages", LOG_ERROR, LOG_TO_ALL);
            free(client_id);
            return;
        }
    }

    // Construct the file path
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s_messages.txt", directory, client->id);

    // Write the received message to the file
    FILE *file = fopen(filepath, "a");
    if (file) {
        fprintf(file, "%s:%s\n", client->id, buffer);
        fclose(file);
    } else {
        output_log("Error writing message from %s to file", LOG_ERROR, LOG_TO_ALL, client->id);
    }

    free(client_id); // Free the dynamically allocated client ID
}
