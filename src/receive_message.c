#include <stdio.h>
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

extern ClientHashTable hash_table;

int write_to_client_log(int client_socket, char *message) {
    output_log("Getting client ID from socket\n", LOG_DEBUG, LOG_TO_CONSOLE);

    // Find the client in the hash table using the socket
    char *client_id = generate_client_id_from_socket(client_socket);
    if (client_id == NULL) {
        output_log("Error: Could not generate client ID for socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        return -1; // Indicate failure
    }

    output_log("OK - Got ID from socket\n", LOG_DEBUG, LOG_TO_CONSOLE);
    output_log("Getting client struct from ID\n", LOG_DEBUG, LOG_TO_CONSOLE);

    Client *client = find_client(&hash_table, client_id);
    if (client == NULL) {
        output_log("Error: Client with ID %s not found\n", LOG_ERROR, LOG_TO_ALL, client_id);
        free(client_id);
        return -1; // Indicate failure
    }

    output_log("OK - Got client struct from ID\n", LOG_DEBUG, LOG_TO_CONSOLE);

    // Define the directory for client message files
    const char *directory = "client_messages";

    // Create the directory if it doesn't exist
    struct stat st = {0};
    if (stat(directory, &st) == -1) {
        if (mkdir(directory, 0755) != 0) {
            output_log("Error creating directory for client messages\n", LOG_ERROR, LOG_TO_ALL);
            free(client_id);
            return -1; // Indicate failure
        }
    }

    // Construct the file path
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s_messages.txt", directory, client->id);

    output_log("Writing to client log\n", LOG_DEBUG, LOG_TO_CONSOLE);

    // Write the received message to the file
    FILE *file = fopen(filepath, "a");
    if (file) {
        fprintf(file, "%s:%s\n", client->id, message);
        fclose(file);
        output_log("Message from client %s on socket %d written to file\n", LOG_INFO, LOG_TO_ALL, client->id, client_socket);
    } else {
        output_log("Error writing message from %s to file\n", LOG_ERROR, LOG_TO_ALL, client->id);
        free(client_id);
        return -1; // Indicate failure
    }

    output_log("OK - Wrote to client log file\n", LOG_DEBUG, LOG_TO_CONSOLE);

    free(client_id); // Free the dynamically allocated client ID
}

int receive_message_server(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    output_log("Received message from network\n", LOG_DEBUG, LOG_TO_CONSOLE);

    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            output_log("Client disconnected: socket %d\n", LOG_INFO, LOG_TO_ALL, client_socket);
        } else {
            output_log("Error receiving message from client socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        }
        return -1; // Indicate failure
    }

    buffer[bytes_received] = '\0'; // Null-terminate the received message

    return write_to_client_log(client_socket, buffer);
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