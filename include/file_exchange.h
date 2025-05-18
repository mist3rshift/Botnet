#ifndef FILE_EXCHANGE_H
#define FILE_EXCHANGE_H
#define FILE_CHUNK_SIZE 4096

int receive_file(int socket, const char *filename );
bool send_file(int socket,const char *filename);

#endif 