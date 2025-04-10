#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "../lib/mongoose.h"

void handle_request(struct mg_connection *c, int ev, void *ev_data);
void handle_send_command(struct mg_connection *c, struct mg_http_message *hm);
void handle_list_bots(struct mg_connection *c, struct mg_http_message *hm);
void handle_server_status(struct mg_connection *c, struct mg_http_message *hm);
void handle_get_bot_file(struct mg_connection *c, struct mg_http_message *hm); // New function
void *start_web_interface(void *arg);

#endif
