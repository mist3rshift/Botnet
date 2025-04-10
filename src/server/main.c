//
// Created by angaros on 3/10/2025.
//

#include "../../lib/mongoose.h"
#include "../../include/server/server_errors.h"
#include "../../include/logging.h"
#include "../../include/server/server_constants.h"
#include "../../include/server/hash_table.h"
#include "../../include/send_message.h"
#include "../../include/launch_arguments.h"
#include "../../include/web_server.h"

#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int initialize_server_socket(int port);
void handle_client_connections(int serverSocket);
char *generate_client_id_from_socket(int client_socket);

int main(int argc, char *argv[])
{
    /*
    output_log("%s\n", LOG_DEBUG, LOG_TO_ALL, "Showcase Debug log");
    output_log("%s\n", LOG_INFO, LOG_TO_ALL, "Showcase Info log");
    output_log("%s\n", LOG_WARNING, LOG_TO_ALL, "Showcase Warning log");
    output_log("%s\n", LOG_ERROR, LOG_TO_ALL, "Showcase Error log");
    server_setup_failed_exception("Failed to setup server"); // Showcase custom error
    */

    parse_arguments(argc, argv);

    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);
    int serverSocket, dialogSocket;

    serverSocket = initialize_server_socket(atoi(DEFAULT_SERVER_PORT));

    output_log("Listening on port %s...\n", LOG_INFO, LOG_TO_ALL, DEFAULT_SERVER_PORT);

    pthread_t web_thread;
    if (pthread_create(&web_thread, NULL, start_web_interface, NULL) != 0) {
        // Start web serv in another thread
        fprintf(stderr, "Failed to create web interface thread\n");
        return 1;
    }

    handle_client_connections(serverSocket); // Launch main server sock

    // If above ends (somehow?) then join and close web server nicely
    pthread_join(web_thread, NULL);

    close(serverSocket); // Close the socket cleanly too
    
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
        server_setup_failed_exception("initialize_server_socket : Error while creating server socket");
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
        server_socket_bind_exception("initialize_server_socket : Error while binding server socket\n");
        exit(1);
    }

    // Start listening for incoming connections
    if (listen(serverSocket,MAX_CLIENTS ) < 0)
    {
        server_setup_failed_exception("initialize_server_socket : Error before server socket listening\n");
        exit(1);
    }

    return serverSocket;
}

// Function to handle multiple client connections using select
void handle_client_connections(int serverSocket)
{
    fd_set master_set, read_set;
    int max_fd = serverSocket;
    int client_sockets[MAX_CLIENTS] = {0};
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    // &hash_table defined globally within hash_table.c
    init_client_table(&hash_table);

    FD_ZERO(&master_set);
    FD_SET(serverSocket, &master_set);

    while (1)
    {
        read_set = master_set;

        // Use select to monitor sockets
        if (select(max_fd + 1, &read_set, NULL, NULL, NULL) < 0)
        {
            output_log("%s\n", LOG_ERROR, LOG_TO_ALL, " handle_client_connections : Error in select");
        }

        // Check for new connections
        // If the server socket is ready for reading, it indicates a new client connection. 
        if (FD_ISSET(serverSocket, &read_set))
        {
            // Accept the new connection
            int new_socket = accept(serverSocket, (struct sockaddr *)&cli_addr, &clilen);
            if (new_socket < 0)
            {
                output_log("%s\n", LOG_ERROR, LOG_TO_ALL, " handle_client_connections : Error accepting new connection");
                continue;
            }

            output_log("New connection from %s:%d\n", LOG_INFO, LOG_TO_ALL, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

            // Add the new socket to the client list
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    FD_SET(new_socket, &master_set); // Add the new socket to the master set ( list for all sockets)
                    if (new_socket > max_fd)
                        max_fd = new_socket;

                    // Add the client to the hash table
                    char *client_id = generate_client_id_from_socket(new_socket);
                    if (client_id == NULL)
                    {
                        output_log("%s\n", LOG_ERROR, LOG_TO_ALL, "handle_client_connections : Error generating client ID");
                        continue;
                    }
                    add_client(&hash_table, client_id, new_socket, LISTENING);
                    print_client_table(&hash_table);
                    break;
                }
            }
        }

        // Check for activity on client sockets
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int client_socket = client_sockets[i];
            if (client_socket > 0 && FD_ISSET(client_socket, &read_set))
            {
                // Handle incoming data from the client
                // Here, we can use recv() to read data from the client socket
                // and process it accordingly.
                char buffer[1024];
                int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
                char *client_id = generate_client_id_from_socket(client_socket);
                if (client_id == NULL)
                {
                    output_log("handle_client_connections : Error generating client ID\n", LOG_ERROR, LOG_TO_ALL);
                    continue;
                }
                if (bytes_read <= 0)
                {
                    // Client disconnected
                    output_log("Client disconnected: socket %d\n", LOG_DEBUG, LOG_TO_CONSOLE, client_socket);
                    close(client_socket);
                    FD_CLR(client_socket, &master_set);
                    client_sockets[i] = 0;

                    // Update the client's state in the hash table
                    Client *client = find_client(&hash_table, client_id);
                    if (client)
                    {
                        client->state = UNREACHABLE;
                        //remove_client(&hash_table, client->id);
                    }
                }
                else
                {
                    // Handle client message
                    buffer[bytes_read] = '\0';
                    output_log("Received from client %d: %s\n", LOG_INFO, LOG_TO_ALL, client_socket, buffer);

                    // Update the client's state to ACTIVE
                    Client *client = find_client(&hash_table, client_id);
                    if (client)
                    {
                        client->state = ACTIVE;
                    }

                    if (client_socket <= 0)
                    {
                        // Ensure the socket is valid before sending a message back
                        output_log("Invalid client socket: %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
                        continue;
                    }

                    send_message(client_socket, buffer);
                }
            }
        }
    }

    // Cleanup the hash table
    print_client_table(&hash_table);
    free_client_table(&hash_table);

}

char *generate_client_id_from_socket(int client_socket){
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char *client_id = malloc(22* sizeof(char)); // 15 for IP + 1 for ':' + 5 for port + 1 for '\0'

    Client *client = find_client_by_socket(&hash_table, client_socket);
    if (client) {
            snprintf(client_id, 22, "%s", client->id);
            return client_id;
    }else{
        if (getpeername(client_socket, (struct sockaddr*)&addr, &addr_len) == -1) {
            output_log("generate_client_id_from_socket : Error getting peer name\n", LOG_ERROR, LOG_TO_ALL);
        free(client_id);
        return NULL;
        }else{                
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
            int port = ntohs(addr.sin_port);
            snprintf(client_id, 22, "%s:%d", ip, port);
            return client_id;
        }
            
    }

}

