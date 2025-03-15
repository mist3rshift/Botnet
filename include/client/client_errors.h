//
// Created by angaros on 3/10/2025.
//

#ifndef CLIENT_ERRORS_H
#define CLIENT_ERRORS_H

// Define connection errors with 1xx
#define SERVER_IP_NOT_FOUND 100
#define SERVER_PORT_NOT_FOUND 101

int server_ip_not_found_exception(char *error_message);

#endif //CLIENT_ERRORS_H
