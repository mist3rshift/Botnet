//
// Created by angaros on 3/10/2025.
//

#include <stddef.h>

#include "../../include/server/server_errors.h"
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
    /*
    output_log("%s\n", LOG_DEBUG, LOG_TO_ALL, "Showcase Debug log");
    output_log("%s\n", LOG_INFO, LOG_TO_ALL, "Showcase Info log");
    output_log("%s\n", LOG_WARNING, LOG_TO_ALL, "Showcase Warning log");
    output_log("%s\n", LOG_ERROR, LOG_TO_ALL, "Showcase Error log");
    server_setup_failed_exception("Failed to setup server"); // Showcase custom error
    */

    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    int clilen = sizeof(cli_addr);
    int serverSocket, dialogSocket;
    char buffer[1024];
    int n;

    // creation of the AF_INET (IPV4) server socket
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("erreur socket");
        exit(1);
    }

    // filling of our "sockaddr_in" structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // listen on any ports
    serv_addr.sin_port = htons((ushort)atoi(DEFAULT_SERVER_PORT));

    // binding between the created socket and the sock_addr_in structure
    if (bind(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("servecho: erreur bind\n");
        exit(1);
    }

    // manage number of pending requests for server socket (3 for now)
    if (listen(serverSocket, 3) < 0)
    {
        perror("servecho: erreur listen\n");
        exit(1);
    }

    printf("Listening on port %s...\n", DEFAULT_SERVER_PORT);

    // new socket creation when a new client connection occurs
    dialogSocket = accept(serverSocket, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    if (dialogSocket < 0)
    {
        perror("servecho : erreur accept\n");
        exit(1);
    }

    printf("New connexion from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    return 0;
}