//
// Created by angaros on 3/31/2025.
//

#include <stdio.h>

#include "../include/commands.h"
#include "../include/logging.h"
#include "../include/client/client_utils.h"
#include <string.h>

int test_serialize(){
    Command *cmd = build_command("test3", 1000, "echo", 0, time(NULL), "Hello World", "-a", "-b", NULL);
    char buffer[1025];
    serialize_command(cmd, buffer, sizeof(buffer));
    output_log("Serialized Command : %s\n", LOG_INFO, LOG_TO_CONSOLE, buffer);

    Command deserialized_cmd;
    deserialize_command(buffer, &deserialized_cmd);

    // Verify the deserialized command
    printf("Deserialized Command:\n");
    printf("  Order Type: %d\n", deserialized_cmd.order_type);
    printf("  Command ID: %s\n", deserialized_cmd.cmd_id);
    printf("  Delay: %d\n", deserialized_cmd.delay);
    printf("  Program: %s\n", deserialized_cmd.program);
    printf("  Expected Exit Code: %d\n", deserialized_cmd.expected_exit_code);
    printf("  Timestamp: %ld\n", deserialized_cmd.timestamp);
    printf("  Params: ");
    if (deserialized_cmd.params) {
        for (size_t i = 0; deserialized_cmd.params[i] != NULL; ++i) {
            printf("%s ", deserialized_cmd.params[i]);
        }
    }
    printf("\n");
    return 0;
}

int main() {
    int total = 0;
    int errors = 0;
    Command *cmd = build_command("test1", 1000, "echo", 0, time(NULL), "Hello World", "-a", "-b", NULL);
    char result_buffer[4096] = {0};
    int exit_code = execute_command(cmd, result_buffer, sizeof(result_buffer));
    total += 1;
    if (exit_code == 0) {
        output_log("Test Successful - Command result : %s\n", LOG_INFO, LOG_TO_CONSOLE, result_buffer);
    } else {
        errors += 1;
        output_log("Test Error - command returned string %s, code %d\n", LOG_ERROR, LOG_TO_CONSOLE, result_buffer, exit_code);
    }
    cmd = build_command("test2", 1000, "echo", 0, time(NULL), "Hello World", "-a", "-b", NULL);
    exit_code = execute_command(cmd, result_buffer, sizeof(result_buffer));
    total += 1;
    if (exit_code == 0) {
        output_log("Test Successful - Command result : %s\n", LOG_INFO, LOG_TO_CONSOLE, result_buffer);
    } else {
        errors += 1;
        output_log("Test Error - command returned string %s, code %d\n", LOG_ERROR, LOG_TO_CONSOLE, result_buffer, exit_code);
    }
    total += 1;
    if (test_serialize() == 0) {
        output_log("Test Successful - serialization\n", LOG_INFO, LOG_TO_CONSOLE);
    } else {
        errors += 1;
        output_log("Test Error - serialization failed with errors\n", LOG_ERROR, LOG_TO_CONSOLE);
    }
    if (errors >= 1) {
        output_log("Some tests have failed : %d / %d", LOG_WARNING, LOG_TO_CONSOLE, errors, total);
        return -1;
    } else {
        output_log("All tests passed : %d", LOG_INFO, LOG_TO_CONSOLE, total);
    }
    return 0;
}
