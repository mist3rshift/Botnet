#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

/**
 * Sends a message to the server.
 * 
 * @param sockfd The socket file descriptor connected to the server.
 * @param message The message to send to the server.
 * @return 0 on success, -1 on failure.
 */
int send_message(int sockfd, const char *message)
{
    if (message == NULL)
    {
        output_log("Message is NULL\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    // Send the message to the server
    ssize_t bytes_sent = send(sockfd, message, strlen(message), 0);
    if (bytes_sent < 0)
    {
        perror("Error sending message");
        output_log("Failed to send message to socket id %d\n", LOG_ERROR, LOG_TO_ALL, sockfd);

        // Check if the socket is still valid
        if (errno == ECONNRESET || errno == EPIPE)
        {
            output_log("Connection to the receiver was lost\n", LOG_WARNING, LOG_TO_ALL);
        }

        return -1;
    }

    output_log("Message sent to the receiver successfully!\n", LOG_INFO, LOG_TO_ALL);
    return 0;
}