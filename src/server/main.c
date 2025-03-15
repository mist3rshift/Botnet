//
// Created by angaros on 3/10/2025.
//
#ifdef _LINUX
#include <socket.h>
#elseif _WIN32
#include <winsock2.h>
#endif

#include <stddef.h>

#include "../../include/server/server_errors.h"
#include "../../include/logging.h"

int main() {
    output_log("%s\n", LOG_DEBUG, LOG_TO_ALL, "Showcase Debug log");
    output_log("%s\n", LOG_INFO, LOG_TO_ALL, "Showcase Info log");
    output_log("%s\n", LOG_WARNING, LOG_TO_ALL, "Showcase Warning log");
    output_log("%s\n", LOG_ERROR, LOG_TO_ALL, "Showcase Error log");
    server_setup_failed_exception("Failed to setup server"); // Showcase custom error
    return 0;
}