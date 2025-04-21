#include "../../include/logging.h"
#include "../../include/server/hash_table.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

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