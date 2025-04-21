#ifndef RECEIVE_MESSAGE_H
#define RECEIVE_MESSAGE_H

int write_to_client_log(int client_socket, char *message);
int receive_message_server(int client_socket);
int receive_message_client(int client_socket, char *buffer, size_t buffer_size);

#endif
