#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "../lib/mongoose.h"
#include "../lib/cJSON.h"
#include "../include/server/hash_table.h"

void handle_request(struct mg_connection *c, int ev, void *ev_data);
void handle_send_command(struct mg_connection *c, struct mg_http_message *hm);
void handle_list_bots(struct mg_connection *c, struct mg_http_message *hm);
// Use dependency injection for tests
void handle_server_status(
    struct mg_connection *c,
    struct mg_http_message *hm,
    int (*socket_func)(int, int, int),
    int (*connect_func)(int, const struct sockaddr *, socklen_t),
    void (*reply_func)(struct mg_connection *, int, const char *, const char *, ...)
);
void handle_update_bots(struct mg_connection *c, struct mg_http_message *hm);
void handle_sysinfo_bots(struct mg_connection *c, struct mg_http_message *hm);
void handle_get_bot_file(struct mg_connection *c, struct mg_http_message *hm); // New function
void *start_web_interface(void *arg);
cJSON *execute_command_and_fetch_result(Client *client, const char *cmd_id, const char *program, const char *params, int delay, int expected_code);

#endif
