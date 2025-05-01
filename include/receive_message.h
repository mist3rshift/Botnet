#ifndef RECEIVE_MESSAGE_H
#define RECEIVE_MESSAGE_H

#include <sys/types.h> // For ssize_t
#include "server/hash_table.h"

int receive_message_server(
    int client_socket,
    char *(*generate_client_id)(int),
    Client *(*find_client_func)(ClientHashTable *, const char *),
    ssize_t (*recv_func)(int, void *, size_t, int) // Add recv_func argument
);

int initialize_client_log(const char *client_id);
int write_to_client_log(Client *client, const char *message);
int receive_message_client(int client_socket, char *buffer, size_t buffer_size);

#endif // RECEIVE_MESSAGE_H
