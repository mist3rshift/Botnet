//
// Created by angaros on 3/10/2025.
//

#include <stddef.h>

#include "../../include/server/server_errors.h"
#include "../../include/logging.h"
#include "../../include/server/server_constants.h"
#include "../../include/server/hash_table.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int initialize_server_socket(int port);
int main()
{
    /*
    output_log("%s\n", LOG_DEBUG, LOG_TO_ALL, "Showcase Debug log");
    output_log("%s\n", LOG_INFO, LOG_TO_ALL, "Showcase Info log");
    output_log("%s\n", LOG_WARNING, LOG_TO_ALL, "Showcase Warning log");
    output_log("%s\n", LOG_ERROR, LOG_TO_ALL, "Showcase Error log");
    server_setup_failed_exception("Failed to setup server"); // Showcase custom error
    */

    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);
    int serverSocket, dialogSocket;

    serverSocket = initialize_server_socket(atoi(DEFAULT_SERVER_PORT));

    printf("Listening on port %s...\n", DEFAULT_SERVER_PORT);

    // new socket creation when a new client connection occurs
    dialogSocket = accept(serverSocket, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
    if (dialogSocket < 0)
    {
        server_setup_failed_exception("Error while accepting a new connection from a client\n");
        exit(1);
    }

    printf("New connexion from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    return 0;
}

// Function to initialize the server socket
int initialize_server_socket(int port)
{
    int serverSocket;
    struct sockaddr_in serv_addr;

    // Create the server socket
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        server_setup_failed_exception("Error while creating server socket");
        exit(1);
    }

    // Configure the server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any interface
    serv_addr.sin_port = htons(port);

    // Bind the socket to the address
    if (bind(serverSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        server_socket_bind_exception("Error while binding server socket\n");
        exit(1);
    }

    // Start listening for incoming connections
    if (listen(serverSocket,MAX_CLIENTS ) < 0)
    {
        server_setup_failed_exception("Error before server socket listening\n");
        exit(1);
    }

    return serverSocket;
}