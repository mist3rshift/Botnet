#ifndef SEND_MESSAGE_H
#define SEND_MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "../include/commands.h"

/**
 * Sends a message to the server.
 * 
 * @param sockfd The socket file descriptor connected to the server.
 * @param message The message to send to the server.
 * @return 0 on success, -1 on failure.
 */
int send_message(int sockfd, const char *message);
int send_command(int sockfd, const Command *cmd);

#endif