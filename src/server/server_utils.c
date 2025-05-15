#include "../../include/logging.h"
#include "../../include/server/hash_table.h"
#include "../../include/server/server_utils.h"
#include "../../include/file_exchange.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <endian.h>

char *generate_client_id_from_socket(int client_socket)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char *client_id = malloc(22 * sizeof(char)); // 15 for IP + 1 for ':' + 5 for port + 1 for '\0'

    Client *client = find_client_by_socket(&hash_table, client_socket);
    if (client)
    {
        snprintf(client_id, 22, "%s", client->id);
        return client_id;
    }
    else
    {
        if (getpeername(client_socket, (struct sockaddr *)&addr, &addr_len) == -1)
        {
            output_log("generate_client_id_from_socket : Error getting peer name\n", LOG_ERROR, LOG_TO_ALL);
            free(client_id);
            return NULL;
        }
        else
        {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
            int port = ntohs(addr.sin_port);
            snprintf(client_id, 22, "%s:%d", ip, port);
            return client_id;
        }
    }
}


int handle_upload(int client_socket, const char *filename) {
    // Générer l'ID du client à partir du socket
    char *client_id = generate_client_id_from_socket(client_socket);
    if (!client_id) {
        output_log("Error: Could not generate client ID for socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        return -1;
    }

    // Construire le chemin complet du fichier dans le répertoire du client
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "uploads/%s", client_id);


    if (receive_file(client_socket,filepath,filename) !=0) {
        output_log("Error: Could not receive file %s\n", LOG_ERROR, LOG_TO_ALL, filepath);
        free(client_id);
        return -1;
    }
    return 0;
}
