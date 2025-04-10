#ifndef SERVER_MAIN_H
#define SERVER_MAIN_H

int initialize_server_socket(int port);
void handle_client_connections(int serverSocket);
char *generate_client_id_from_socket(int client_socket);

#endif
