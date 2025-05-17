#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <stdint.h>
#include <endian.h>

#include "../include/logging.h"
#include "../include/send_message.h"
#include "../include/file_exchange.h"


//RECEIVING PART

void ensure_directory_exists(const char *filepath) {

    struct stat st = {0};
    if (stat(filepath, &st) == -1) {
        mkdir("uploads", 0755);  // Ensure root uploads folder exists
        mkdir(filepath, 0755);       // Create folder
    }
}

int receive_file(int socket,const char* filepath, const char *filename ){

    // Assurez-vous que le répertoire du client existe
    ensure_directory_exists(filepath);

    // Construire le chemin complet du fichier dans le répertoire du client
    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", filepath, filename);

    // Ouvrir le fichier pour l'écriture
    FILE *fp = fopen(fullpath, "wb");
    if (!fp) {
        output_log("Error: Could not open file %s for writing\n", LOG_ERROR, LOG_TO_ALL, fullpath);
        return -1;
    }

    char buffer[FILE_CHUNK_SIZE];

    // Lire la taille du fichier (8 octets)
    uint64_t filesize_net;
    ssize_t n = recv(socket, &filesize_net, sizeof(filesize_net), MSG_WAITALL);
    if (n != sizeof(filesize_net)) {
        output_log("Error: Could not read file size\n", LOG_ERROR, LOG_TO_ALL);
        fclose(fp);
        return -1;
    }
    uint64_t filesize = be64toh(filesize_net);

    // Lire exactement filesize octets
    uint64_t total_received = 0;
    while (total_received < filesize) {
        size_t to_read = (filesize - total_received > FILE_CHUNK_SIZE) ? FILE_CHUNK_SIZE : (filesize - total_received);
        ssize_t bytes_received = recv(socket, buffer, to_read, 0);
        if (bytes_received <= 0) break;
        fwrite(buffer, 1, bytes_received, fp);
        total_received += bytes_received;
        output_log("handle_upload : Received EOF signal from client socket %d\n", LOG_DEBUG, LOG_TO_ALL, socket);

    }

    fclose(fp);

    if (total_received != filesize) {
        output_log("Error: File transfer incomplete\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    output_log("File '%s' received successfully and saved to '%s'\n", LOG_INFO, LOG_TO_ALL,fullpath);
    return 0;

}

// SENDING PART
void send_file(int socket,const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        send_message(socket, "ERROR: File not found");
        return;
    }
    // Récupérer la taille du fichier
    struct stat st;
    if (stat(filename, &st) != 0) {
        output_log("send_file : Error getting file stats", LOG_INFO, LOG_TO_ALL, filename);
        fclose(fp);
        return;
    }
    long filesize = st.st_size;
    long filesize_net = htobe64(filesize); // conversion réseau
    
    // Envoie du nom de fichier
    char header[1024];
    snprintf(header, sizeof(header), "UPLOAD%s", filename);
    if (send(socket, header, strlen(header), 0) < 0) {
        output_log("send_file : Error sending file data header", LOG_INFO, LOG_TO_ALL, filename);
        fclose(fp);
        return;
    }
    usleep(100000); // petite pause pour éviter que le header soit collé au reste
    
    // Envoie la taille du fichier (8 octets)
    if (send(socket, &filesize_net, sizeof(filesize_net), 0) < 0) {
        output_log("send_file : Error sending file size", LOG_INFO, LOG_TO_ALL, filename);
        fclose(fp);
        return;
    }

    // Envoie des données en chunks de taille 4096
    char buffer[FILE_CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, FILE_CHUNK_SIZE, fp)) > 0) { //boubler jusqu'à EOF
        if (send(socket, buffer, bytes_read, 0) < 0) {
            output_log("send_file : Error sending file data",LOG_INFO, LOG_TO_ALL, filename);
            fclose(fp);
            return;
        }
    }

    fclose(fp);
    char eof_signal = EOF;
    if (send(socket, &eof_signal, 1, 0) < 0) {
        output_log("send_file : Error sending EOF", LOG_INFO, LOG_TO_ALL, filename);
        return;
    }
    output_log("File '%s' sent successfully\n", LOG_INFO, LOG_TO_ALL, filename);
    return;
}
