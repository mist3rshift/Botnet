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
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        // Client disconnected or error occurred
        if (bytes_read == 0) {
            output_log("Client disconnected: socket %d\n", LOG_INFO, LOG_TO_ALL, client_socket);
        } else {
            output_log("Error reading from socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        }

        // Update the client's state to DISCONNECTED
        char *client_id = generate_client_id_from_socket(client_socket);
        if (client_id != NULL) {
            Client *client = find_client(&hash_table, client_id);
            if (client) {
                client->state = UNREACHABLE;
            }
            free(client_id);
        }

        // Close the socket
        close(client_socket);
        return;
    }

    // Null-terminate the buffer and process the message
    buffer[bytes_read] = '\0';
    output_log("Received message from socket %d: %s\n", LOG_INFO, LOG_TO_ALL, client_socket, buffer);

    // Update the client's state to ACTIVE
    char *client_id = generate_client_id_from_socket(client_socket);
    if (client_id != NULL) {
        Client *client = find_client(&hash_table, client_id);
        if (client) {
            client->state = ACTIVE;
        }
        free(client_id);
    }

    // Process the message (this can be extended as needed)
    // For example, you could parse commands or forward messages to other clients
}
