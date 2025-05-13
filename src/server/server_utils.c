#include "../../include/logging.h"
#include "../../include/server/hash_table.h"
#include "../../include/server/server_utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/stat.h>

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

void ensure_client_directory_exists(const char *client_id) {
    char path[256];
    snprintf(path, sizeof(path), "uploads/%s", client_id);

    struct stat st = {0};
    if (stat(path, &st) == -1) {
        mkdir("uploads", 0755);  // Ensure root uploads folder exists
        mkdir(path, 0755);       // Create client folder
    }
}

int handle_upload(int client_socket, const char *filename) {
    // Générer l'ID du client à partir du socket
    char *client_id = generate_client_id_from_socket(client_socket);
    if (!client_id) {
        output_log("Error: Could not generate client ID for socket %d\n", LOG_ERROR, LOG_TO_ALL, client_socket);
        return -1;
    }

    // Assurez-vous que le répertoire du client existe
    ensure_client_directory_exists(client_id);

    // Construire le chemin complet du fichier dans le répertoire du client
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "uploads/%s/%s", client_id, filename);

    // Ouvrir le fichier pour l'écriture
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        output_log("Error: Could not open file %s for writing\n", LOG_ERROR, LOG_TO_ALL, filepath);
        free(client_id);
        return -1;
    }

    char buffer[BLOCK_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        // Vérifiez si "EOF" est présent dans les données reçues
        char *eof_position = strstr(buffer, "EOF");
        if (eof_position) {
            // Écrire uniquement les données avant "EOF"
            size_t data_length = eof_position - buffer;
            if (data_length > 0) {
                fwrite(buffer, 1, data_length, fp);
            }
            output_log("handle_upload : Received EOF signal from client socket %d\n", LOG_DEBUG, LOG_TO_ALL, client_socket);
            break; // Arrêtez la réception
        }

        // Écrire toutes les données reçues dans le fichier
        fwrite(buffer, 1, bytes_received, fp);
    }

    fclose(fp);
    free(client_id);

    if (bytes_received < 0) {
        output_log("Error receiving file data from client\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    output_log("File '%s' received successfully and saved to '%s'\n", LOG_INFO, LOG_TO_ALL, filename, filepath);
    return 0;
}
