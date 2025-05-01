#ifndef RECEIVE_MESSAGE_H
#define RECEIVE_MESSAGE_H

#include "server/hash_table.h"

int initialize_client_log(char* client_socket);
int write_to_client_log(Client *client, char *message);
int receive_message_server(int client_socket);
int receive_message_client(int client_socket, char *buffer, size_t buffer_size);

#endif
