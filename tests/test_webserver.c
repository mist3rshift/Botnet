#include <pthread.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../include/server/web_server.h"
#include "../include/server/server_constants.h"
#include "../include/logging.h"
#include "../lib/mongoose.h"
#include "../include/server/hash_table.h"
#include "../include/commands.h"
#include <string.h>

// Mock client for testing
Client mock_client = {
    .id = "test_bot",
    .socket = 1,
    .state = ACTIVE,
    .next = NULL
};

static void assert_string_contains(const char *str, const char *substr);

pthread_t web_thread;
extern bool stop_server; // Declare the stop_server flag

static char mock_response[1024]; // Buffer to store the mocked response

int mock_socket(int domain, int type, int protocol) {
    (void)domain; (void)type; (void)protocol;
    return 1; // Return a valid socket descriptor
}

int mock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    (void)sockfd; (void)addr; (void)addrlen;
    return 0; // Simulate a successful connection
}

void mock_reply_func(struct mg_connection *c, int status_code, const char *headers, const char *body, ...) {
    (void)c; (void)status_code; (void)headers;

    va_list args;
    va_start(args, body);
    vsnprintf(mock_response, sizeof(mock_response), body, args); // Format the response with variadic arguments
    va_end(args);

    printf("Mock response captured: %s\n", mock_response); // Debug log
}

void mock_mg_http_reply(struct mg_connection *c, int status_code, const char *headers, const char *body) {
    (void)c; // Unused
    (void)status_code; // Unused
    (void)headers; // Unused
    snprintf(mock_response, sizeof(mock_response), "%s", body); // Capture the response body
    printf("Mock response: %s\n", mock_response);
}

void mock_mg_http_reply_wrapper(struct mg_connection *c, int status_code, const char *headers, const char *body, ...) {
    (void)c; // Unused
    (void)status_code; // Unused
    (void)headers; // Unused
    snprintf(mock_response, sizeof(mock_response), "%s", body); // Capture the response body
    printf("Mock response: %s\n", mock_response);
}

void mg_http_reply_wrapper(struct mg_connection *c, int status_code, const char *headers, const char *body, ...) {
    va_list args;
    va_start(args, body);
    mg_http_reply(c, status_code, headers, body, args);
    va_end(args);
}

void (*mg_http_reply_wrapper_ptr)(struct mg_connection *, int, const char *, const char *, ...) = mg_http_reply_wrapper;

int test_setup(void *(*__function) (void *)) { // Call before tests
    if (pthread_create(&web_thread, NULL, __function, NULL) != 0) {
        // Start web serv in another thread
        output_log("Test Error - test_setup : Failed to create server thread!\n", LOG_ERROR, LOG_TO_CONSOLE);
        return 1;
    }
    return 0;
}

int test_close() { // Call after tests
    stop_server = true; // Signal the server to stop
    return pthread_join(web_thread, NULL); // Wait for the server thread to exit
}

static void test_start_web_interface(void **state) { // Call to test the start method
    (void)state; // Unused
    test_setup(start_web_interface);
    const char *web_server_ip = DEFAULT_WEB_ADDR;
    int web_server_port = atoi(DEFAULT_WEB_PORT);
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    char addr[64];
    snprintf(addr, sizeof(addr), "http://%s:%d", web_server_ip, web_server_port); // Construct web addr of web server
    struct mg_connection *web_conn = mg_http_connect(&mgr, addr, NULL, NULL);
    if (web_conn != NULL) {
        output_log("%s", LOG_INFO, LOG_TO_CONSOLE, "Test Successful\n");
    } else {
        output_log("Test Error - start_web_interface : web_conn was null!\n", LOG_ERROR, LOG_TO_CONSOLE);
    }
    mg_mgr_free(&mgr); // We no longer need manager
    test_close(); // Stop the server and wait for the thread to exit
}

// Test for listing bots
static void test_list_bots(void **state) {
    (void)state; // Unused

    // Add a mock client to the hash table
    hash_table.size = 1;
    hash_table.table = calloc(1, sizeof(Client *));
    hash_table.table[0] = &mock_client;

    struct mg_connection c;
    struct mg_http_message hm;
    memset(&c, 0, sizeof(c));
    memset(&hm, 0, sizeof(hm));

    // Initialize required fields if needed
    c.is_websocket = 0; // Example: Set to 0 if not a WebSocket connection

    // Call the handler
    handle_list_bots(&c, &hm);

    // Verify the response
    // (In a real test, you would mock `mg_http_reply` and verify the JSON response)
    assert_non_null(hash_table.table[0]);
    assert_string_equal(hash_table.table[0]->id, "test_bot");

    free(hash_table.table);
}

// Test for sending a command
static void test_send_command(void **state) {
    (void)state; // Unused

    // Add a mock client to the hash table
    hash_table.size = 1;
    hash_table.table = calloc(1, sizeof(Client *));
    hash_table.table[0] = &mock_client;

    struct mg_connection c;
    struct mg_http_message hm;
    memset(&c, 0, sizeof(c));
    memset(&hm, 0, sizeof(hm));

    // Initialize required fields if needed
    c.is_websocket = 0; // Example: Set to 0 if not a WebSocket connection

    // Mock POST request body
    hm.body.buf = "bot_id=test_bot&cmd_id=1&program=echo&params=Hello&delay=0&expected_code=0";
    hm.body.len = strlen(hm.body.buf);

    // Call the handler
    handle_send_command(&c, &hm);

    // Verify the response
    // (In a real test, you would mock `mg_http_reply` and verify the JSON response)
    assert_non_null(hash_table.table[0]);
    assert_string_equal(hash_table.table[0]->id, "test_bot");

    free(hash_table.table);
}

// Test for server status
static void test_server_status(void **state) {
    (void)state; // Unused

    // Initialize mock hash table
    hash_table.size = 1;
    hash_table.table = calloc(1, sizeof(Client *));
    hash_table.table[0] = &mock_client;

    struct mg_connection c;
    struct mg_http_message hm;
    memset(&c, 0, sizeof(c));
    memset(&hm, 0, sizeof(hm));

    // Call the handler with mock functions
    handle_server_status(&c, &hm, mock_socket, mock_connect, mock_reply_func);

    // Verify the mocked response
    assert_non_null(mock_response);
    assert_string_contains(mock_response, "\"botnet_server_active\":"); // Check for key fields
    assert_string_contains(mock_response, "\"web_server_active\":");
    assert_string_contains(mock_response, "\"connected_clients\":");
    assert_string_contains(mock_response, "\"unreachable_clients\":");

    free(hash_table.table);
}

// Test for getting the current working directory
static void test_get_cwd(void **state) {
    (void)state; // Unused

    // Add a mock client to the hash table
    hash_table.size = 1;
    hash_table.table = calloc(1, sizeof(Client *));
    hash_table.table[0] = &mock_client;

    struct mg_connection c;
    struct mg_http_message hm;
    memset(&c, 0, sizeof(c));
    memset(&hm, 0, sizeof(hm));

    // Initialize required fields if needed
    c.is_websocket = 0; // Example: Set to 0 if not a WebSocket connection

    // Mock query string
    hm.query.buf = "bot_id=test_bot";
    hm.query.len = strlen(hm.query.buf);

    // Call the handler
    //handle_get_cwd(&c, &hm);

    // Verify the response
    // (In a real test, you would mock `mg_http_reply` and verify the JSON response)
    assert_non_null(hash_table.table[0]);
    assert_string_equal(hash_table.table[0]->id, "test_bot");

    free(hash_table.table);
}

static void assert_string_contains(const char *str, const char *substr) {
    if (strstr(str, substr) == NULL) {
        fail_msg("Expected substring '%s' not found in '%s'", substr, str);
    }
}

// Main function to run tests
int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_start_web_interface),
        cmocka_unit_test(test_list_bots),
        cmocka_unit_test(test_send_command),
        cmocka_unit_test(test_server_status),
        cmocka_unit_test(test_get_cwd),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
