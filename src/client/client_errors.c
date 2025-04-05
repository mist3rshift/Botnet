//
// Created by angaros on 3/10/2025.
//

#include <stdio.h>
#include <stdlib.h>

#include "../../include/client/client_errors.h"
#include "../../include/logging.h"

int client_setup_failed_exception(char *error_message)
{
    output_log("%s\n", LOG_ERROR, LOG_TO_ALL, error_message);
    exit(GENERIC_SETUP_FAILED_EXCEPTION);
}

int server_ip_not_found_exception(char *error_message)
{
    output_log("%s\n", LOG_ERROR, LOG_TO_ALL, error_message);
    exit(SERVER_IP_NOT_FOUND);
}
