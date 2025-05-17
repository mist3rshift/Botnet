//
// Created by angaros on 3/10/2025.
//
#include "../../include/client/client_errors.h"
#include "../../include/logging.h"
#include "../../include/server/server_constants.h"
#include "../../include/send_message.h"
#include "../../include/launch_arguments.h" // Include the header for argument parsing
#include "../../include/commands.h"
#include "../../include/client/client_utils.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h> // For fd_set and struct timeval
#include <fcntl.h>

// Set the socket to blocking mode
int set_socket_blocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    init_launch_arguments_defaults();
    parse_arguments(argc, argv); // Parse launch arguments

    struct sockaddr_in serv_addr;
    int sockfd;

    // Create client socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        client_setup_failed_exception("Error while creating client socket");
    }

    set_socket_blocking(sockfd);

    // Fill the server socket structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t)atoi(server_port)); // Use global port
    serv_addr.sin_addr.s_addr = inet_addr(server_address);  // Use global addr

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        client_setup_failed_exception("Error while client trying to connect to the server");
    }

    output_log("Connected to server %s:%s\n", LOG_INFO, LOG_TO_ALL, server_address, server_port);

    // Send an initial message to the server
    //send_message(sockfd, "Hello!");

    // Main loop to monitor and process incoming data
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);

        // Use select to monitor the socket for incoming data
        struct timeval timeout = {5, 0}; // 5-second timeout
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("Error in select");
            break;
        } else if (activity == 0) {
            // Timeout occurred, no data received
            output_log("No data received from server, continuing...\n", LOG_DEBUG, LOG_TO_ALL);
            continue;
        }

        // Check if the socket has data to read
        if (FD_ISSET(sockfd, &read_fds)) {
            output_log("Receiving data, processing message\n", LOG_DEBUG, LOG_TO_CONSOLE);
            // Use receive_and_process_message to handle incoming data
            receive_and_process_message(sockfd);
        }
    }

    // Clean up and close the connection
    close(sockfd);
    output_log("Disconnected from server\n", LOG_INFO, LOG_TO_ALL);

    return 0;
}