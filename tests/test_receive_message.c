#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../include/receive_message.h"
#include "../include/logging.h"
#include <sys/stat.h>  // For mkdir
#include <unistd.h>    // For rmdir

// Mock implementations
char *mock_generate_client_id_from_socket(int client_socket) {
    check_expected(client_socket);
    const char *mock_client_id = (const char *)mock();
    return strdup(mock_client_id); // Dynamically allocate the string
}

Client *mock_find_client(ClientHashTable *hash_table, const char *client_id) {
    check_expected(hash_table);
    check_expected(client_id);
    return (Client *)mock();
}

// Mock recv function
ssize_t mock_recv(int sockfd, void *buf, size_t len, int flags) {
    check_expected(sockfd);
    check_expected(len);
    check_expected(flags);

    const char *mock_message = (const char *)mock();
    size_t message_len = strlen(mock_message);

    if (len < message_len) {
        return -1; // Simulate failure if buffer is too small
    }

    memcpy(buf, mock_message, message_len);
    return message_len; // Simulate successful reception of the message
}

// Test for initialize_client_log
static void test_initialize_client_log(void **state) {
    (void)state; // Unused

    const char *test_dir = CLIENT_MESSAGES_DIR;
    mkdir(test_dir, 0755);

    assert_int_equal(initialize_client_log("test_client"), 0);

    // Verify the file was created
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/test_client_messages.txt", CLIENT_MESSAGES_DIR);
    FILE *file = fopen(filepath, "r");
    assert_non_null(file);
    fclose(file);

    remove(filepath);
    rmdir(test_dir);
}

// Test for write_to_client_log
static void test_write_to_client_log(void **state) {
    (void)state; // Unused

    Client mock_client = {.id = "mock_client_id", .socket = 1};

    const char *test_dir = CLIENT_MESSAGES_DIR;
    mkdir(test_dir, 0755);

    assert_int_equal(initialize_client_log(mock_client.id), 0);
    assert_int_equal(write_to_client_log(&mock_client, "Hello, world!"), 0);

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/mock_client_id_messages.txt", CLIENT_MESSAGES_DIR);
    FILE *file = fopen(filepath, "r");
    assert_non_null(file);

    char buffer[1024];
    fgets(buffer, sizeof(buffer), file);
    assert_string_equal(buffer, "mock_client_id:Hello, world!\n"); // Ensure the expected message matches
    fclose(file);

    remove(filepath);
    rmdir(test_dir);
}

// Test for receive_message_server
static void test_receive_message_server(void **state) {
    (void)state; // Unused

    const char *test_dir = CLIENT_MESSAGES_DIR;
    mkdir(test_dir, 0755);

    // Set up mock behavior
    expect_value(mock_generate_client_id_from_socket, client_socket, 1);
    will_return(mock_generate_client_id_from_socket, "mock_client_id");

    expect_value(mock_find_client, hash_table, &hash_table);
    expect_string(mock_find_client, client_id, "mock_client_id");
    Client mock_client = {.id = "mock_client_id", .socket = 1};
    will_return(mock_find_client, &mock_client);

    // Mock recv behavior
    expect_value(mock_recv, sockfd, 1);
    expect_value(mock_recv, len, 1023); // Expect 1023
    expect_value(mock_recv, flags, 0);
    will_return(mock_recv, "Hello, server!");

    // Test receive_message_server
    assert_int_equal(receive_message_server(1, mock_generate_client_id_from_socket, mock_find_client, mock_recv), 0);

    // Verify the message was written to the log
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/mock_client_id_messages.txt", CLIENT_MESSAGES_DIR);
    FILE *file = fopen(filepath, "r");
    assert_non_null(file);

    char buffer[1024];
    fgets(buffer, sizeof(buffer), file);
    assert_string_equal(buffer, "mock_client_id:Hello, server!\n");
    fclose(file);

    remove(filepath);
    rmdir(test_dir);
}

// Main function to run tests
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_initialize_client_log),
        cmocka_unit_test(test_write_to_client_log),
        cmocka_unit_test(test_receive_message_server),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}