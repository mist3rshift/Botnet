//
// Created by angaros on 3/10/2025.
//
#include "../../include/client/client_errors.h"
#include "../../include/logging.h"
#include "../../include/server/server_constants.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
    // server_ip_not_found_exception("Failed to find server IP");

    struct sockaddr_in serv_addr;
    int sockfd;

    // client socket creation
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error while creating client socket");
        exit(EXIT_FAILURE);
    }

    // filling the server socket structure to which the client will be connected
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((ushort)atoi(DEFAULT_SERVER_PORT));
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connection to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("cliecho : erreur connect");
        exit(1);
    }

    printf("Connected to server %s:%s\n", "127.0.0.1", DEFAULT_SERVER_PORT);

    return 0;
}