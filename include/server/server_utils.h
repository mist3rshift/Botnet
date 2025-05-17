#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H
char *generate_client_id_from_socket(int client_socket);
void ensure_client_directory_exists(const char *client_id);
int handle_download(int client_sock, const char *filename);
#endif