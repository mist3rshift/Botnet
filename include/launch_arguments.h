#ifndef LAUNCH_ARGUMENTS_H
#define LAUNCH_ARGUMENTS_H

#include <netinet/in.h> // For INET_ADDRSTRLEN

// Default server address
extern char server_address[INET_ADDRSTRLEN];

// Function to parse launch arguments
void parse_arguments(int argc, char *argv[]);

#endif // LAUNCH_ARGUMENTS_H