#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H 
#define FILE_CHUNK_SIZE 4096


#include "../commands.h"

enum OrderType get_order_enum_type(const char *buffer);
char* read_client_log_file(int n_last_line);
void execute_order_from_server(int sockfd, enum OrderType order_type, char* buffer);
int parse_and_execute_command(const Command cmd, int sockfd);
void receive_and_process_message(int sockfd);

#endif
