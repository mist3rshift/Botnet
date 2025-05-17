#ifndef LAUNCH_ARGUMENTS_H
#define LAUNCH_ARGUMENTS_H

#include <netinet/in.h> // For INET_ADDRSTRLEN

// Default server address and port
extern char server_address[INET_ADDRSTRLEN];
extern char server_port[6];

// Function to parse launch arguments
void init_launch_arguments_defaults();
void parse_arguments(int argc, char *argv[]);

#endif // LAUNCH_ARGUMENTS_H