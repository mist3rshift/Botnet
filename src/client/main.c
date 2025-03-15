//
// Created by angaros on 3/10/2025.
//
#ifdef _LINUX
#include <>
#elseif _WIN32
#include <winsock2.h>
#endif

#include "../../include/client/client_errors.h"
#include "../../include/logging.h"

int main() {
    server_ip_not_found_exception("Failed to find server IP");
    return 0;
}