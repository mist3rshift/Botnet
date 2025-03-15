//
// Created by angaros on 3/10/2025.
//

#ifndef SERVER_ERRORS_H
#define SERVER_ERRORS_H

// Define setup errors with 1xx
#define GENERIC_SETUP_FAILED_EXCEPTION 100
#define SOCKET_BIND_EXCEPTION 101

int server_setup_failed_exception(char *error_message);
int server_socket_bind_exception(char *error_message);

#endif //SERVER_ERRORS_H
