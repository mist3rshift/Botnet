#ifndef CONSOLE_H
#define CONSOLE_H

void *interactive_menu();
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
void display_bots();
void get_file_from_bot();
void send_command_to_bot();
void get_bot_file();

#endif
