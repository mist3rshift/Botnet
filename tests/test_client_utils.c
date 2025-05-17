#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <assert.h>
#include "../include/client/client_utils.h"

// Mock functions
void mock_send_message(int sockfd, const char *message) {
    printf("Mock send_message called with: %s\n", message);
}

void mock_output_log(const char *format, int log_level, int log_target, ...) {
    va_list args;
    va_start(args, log_target);
    vprintf(format, args);
    va_end(args);
}

// Test function for download_file_to_server
void test_download_file_to_server() {
    // Create a temporary file to simulate the file to download
    const char *test_filename = "test_download_file.txt";
    FILE *test_file = fopen(test_filename, "w");
    assert(test_file != NULL);
    fprintf(test_file, "This is a test file for download.\n");
    fclose(test_file);

    // Create a mock socket
    int sockfd[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == 0);

    // Redirect the output of the mock socket to a temporary file
    int temp_fd = open("mock_socket_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(temp_fd >= 0);
    dup2(sockfd[1], temp_fd);

    // Call the function to test
    download_file_to_server(test_filename, sockfd[0]);

    // Close the mock socket and temporary file
    close(sockfd[0]);
    close(sockfd[1]);
    close(temp_fd);

    // Verify the output
    FILE *output_file = fopen("mock_socket_output.txt", "r");
    assert(output_file != NULL);

    char buffer[1024];
    fread(buffer, 1, sizeof(buffer), output_file);
    fclose(output_file);

    assert(strstr(buffer, "DOOWNLOADtest_download_file.txt") != NULL);
    assert(strstr(buffer, "This is a test file for download.") != NULL);
    assert(strstr(buffer, "EOF") != NULL);

    // Clean up
    remove(test_filename);
    remove("mock_socket_output.txt");

    printf("test_download_file_to_server passed.\n");
}

int main() {
    test_download_file_to_server();
    return 0;
}