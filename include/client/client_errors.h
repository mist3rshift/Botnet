//
// Created by angaros on 3/10/2025.
//

#ifndef CLIENT_ERRORS_H
#define CLIENT_ERRORS_H

// Define connection errors with 1xx
#define GENERIC_SETUP_FAILED_EXCEPTION 100
#define SERVER_IP_NOT_FOUND 101
#define SERVER_PORT_NOT_FOUND 102

int client_setup_failed_exception(char *error_message);
int server_ip_not_found_exception(char *error_message);

#endif // CLIENT_ERRORS_H
