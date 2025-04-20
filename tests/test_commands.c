//
// Created by angaros on 3/31/2025.
//

#include <stdio.h>

#include "../include/commands.h"
#include "../include/logging.h"

int test_serialize(){
    Command *cmd = build_command("test1", 1000, "echo", 0, "Hello World", "-a", "-b", NULL);
    char* buffer[1025];
    serialize_command(cmd, buffer, sizeof(buffer));
    printf("%s", buffer);
    return 0;
}

int main() {
    int total = 0;
    int errors = 0;
    Command *cmd = build_command("test1", 1000, "echo", 0, "Hello World", NULL);
    int return_code = execute_command(cmd);
    total += 1;
    if (return_code == 0) {
        output_log("%s", LOG_INFO, LOG_TO_CONSOLE, "Test Successful\n");
    } else {
        errors += 1;
        output_log("Test Error - command returned exit code %d\n", LOG_ERROR, LOG_TO_CONSOLE, return_code);
    }
    cmd = build_command("test2", 1000, "echo", 0, "Hello World", NULL);
    return_code = execute_command(cmd);
    total += 1;
    if (return_code == 0) {
        output_log("%s", LOG_INFO, LOG_TO_CONSOLE, "Test Successful\n");
    } else {
        errors += 1;
        output_log("Test Error - command returned exit code %d\n", LOG_ERROR, LOG_TO_CONSOLE, return_code);
    }
    if (errors >= 1) {
        output_log("Some tests have failed : %d / %d", LOG_WARNING, LOG_TO_CONSOLE, errors, total);
        return -1;
    } else {
        output_log("All tests passed : %d", LOG_INFO, LOG_TO_CONSOLE, total);
    }
    test_serialize();
    return 0;
}
