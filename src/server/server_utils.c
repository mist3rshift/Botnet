#include "../../include/logging.h"
#include "../../include/server/hash_table.h"
#include "../../include/server/server_utils.h"

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

    // Lire la taille du fichier (8 octets)
    uint64_t filesize_net;
    ssize_t n = recv(client_socket, &filesize_net, sizeof(filesize_net), MSG_WAITALL);
    if (n != sizeof(filesize_net)) {
        output_log("Error: Could not read file size\n", LOG_ERROR, LOG_TO_ALL);
        fclose(fp);
        free(client_id);
        return -1;
    }
    uint64_t filesize = be64toh(filesize_net);

    // Lire exactement filesize octets
    uint64_t total_received = 0;
    while (total_received < filesize) {
        size_t to_read = (filesize - total_received > BLOCK_SIZE) ? BLOCK_SIZE : (filesize - total_received);
        ssize_t bytes_received = recv(client_socket, buffer, to_read, 0);
        if (bytes_received <= 0) break;
        fwrite(buffer, 1, bytes_received, fp);
        total_received += bytes_received;
        output_log("handle_upload : Received EOF signal from client socket %d\n", LOG_DEBUG, LOG_TO_ALL, client_socket);

    }

    fclose(fp);
    free(client_id);

    if (total_received != filesize) {
        output_log("Error: File transfer incomplete\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    output_log("File '%s' received successfully and saved to '%s'\n", LOG_INFO, LOG_TO_ALL, filename, filepath);
    return 0;
}
