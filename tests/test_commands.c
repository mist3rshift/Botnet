//
// Created by angaros on 3/31/2025.
//

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../include/commands.h"
#include "../include/logging.h"
#include "../include/client/client_utils.h"
#include <string.h>

static void test_serialize(void **state) {
    (void)state; // Unused

    // Use a fixed timestamp for testing
    time_t fixed_time = 1672531200;
    Command *cmd = build_command("test3", 1000, "echo", 0, fixed_time, "Hello World", "-a", "-b", NULL);
    char buffer[1025];
    serialize_command(cmd, buffer, sizeof(buffer));
    assert_string_equal(buffer, "0|test3|1000|echo|0|1672531200|Hello World -a -b");
    free_command(cmd);
}

static void test_deserialize(void **state) {
    (void)state; // Unused

    // Use a fixed timestamp for testing
    time_t fixed_time = 1672531200; // Example: Jan 1, 2023, 00:00:00 UTC
    Command *cmd = build_command("test3", 1000, "echo", 0, fixed_time, "Hello World", "-a", "-b", NULL);

    char buffer[1025];
    serialize_command(cmd, buffer, sizeof(buffer));

    Command deserialized_cmd;
    memset(&deserialized_cmd, 0, sizeof(deserialized_cmd)); // Ensure the structure is zeroed out
    deserialize_command(buffer, &deserialized_cmd);

    // Verify the deserialized command
    assert_int_equal(deserialized_cmd.order_type, 0);
    assert_string_equal(deserialized_cmd.cmd_id, "test3");
    assert_int_equal(deserialized_cmd.delay, 1000);
    assert_string_equal(deserialized_cmd.program, "echo");
    assert_int_equal(deserialized_cmd.expected_exit_code, 0);
    assert_int_equal(deserialized_cmd.timestamp, fixed_time); // Verify the timestamp

    // Verify the params field
    assert_non_null(deserialized_cmd.params); // Ensure params is not NULL

    // Check individual parameters
    assert_string_equal(deserialized_cmd.params[0], "Hello");
    assert_string_equal(deserialized_cmd.params[1], "World");
    assert_string_equal(deserialized_cmd.params[2], "-a");
    assert_string_equal(deserialized_cmd.params[3], "-b");
    assert_null(deserialized_cmd.params[4]); // Ensure the array is null-terminated

    // Free the original command
    free_command(cmd);

    // Free the deserialized command
    free_command(&deserialized_cmd);
}

static void test_build_command(void **state) {
    time_t time_now = time(NULL);
    Command *cmd = build_command("test1", 1000, "echo", 0, time_now, "Hello World", "-a", "-b", NULL);
    assert_string_equal(cmd->cmd_id, "test1");
    assert_int_equal(cmd->delay, 1000);
    assert_string_equal(cmd->program, "echo");
    assert_int_equal(cmd->expected_exit_code, 0);
    assert_int_equal(cmd->timestamp, time_now);
}

static void test_execute_command(void **state) {
    Command *cmd = build_command("test1", 1000, "echo", 0, time(NULL), "Hello World", "-a", "-b", NULL);
    char result_buffer[4096] = {0};
    assert_int_equal(execute_command(cmd, result_buffer, sizeof(result_buffer)), 0);
}

static void test_commands_sysinfo(void **state){
    Command **cmds = commands_sysinfo();
    Command *cmd = cmds[4];
    assert_string_equal(cmd->cmd_id, "S_iptables");
    assert_int_equal(cmd->delay, 0);
    assert_string_equal(cmd->program, "iptables");
    assert_int_equal(cmd->expected_exit_code, 0);
    assert_int_equal(cmd->timestamp, time_now);
    assert_string_equal(cmd->params[0], "-L" );
    assert_string_equal(cmd->params[1], NULL);
    free_commands(cmds, 7);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_build_command),
        cmocka_unit_test(test_execute_command),
        cmocka_unit_test(test_serialize),
        cmocka_unit_test(test_deserialize),
        cmocka_unit_test(test_commands_sysinfo)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
