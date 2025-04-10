#include "../include/web_server.h"
#include "logging.h"
#include <pthread.h>
#include "../include/server/server_constants.h"
#include "../include/logging.h"
#include "../lib/mongoose.h"
#include "../include/server/hash_table.h"
#include "../include/commands.h"

pthread_t web_thread;
extern bool stop_server; // Declare the stop_server flag

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

int test_start_web_interface() { // Call to test the start method
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
    return 0;
}

int main() {
    // Run the test
    return test_start_web_interface();
}
