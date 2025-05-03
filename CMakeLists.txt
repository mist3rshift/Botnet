cmake_minimum_required(VERSION 3.22)
project(botnet)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)
link_directories(/usr/lib/x86_64-linux-gnu) # Add linking to other lib (not /usr/lib)
find_package(CMocka REQUIRED) # Need CMocka for tests

add_executable(server # Define the server build
        src/server/main.c
        src/server/server_errors.c
        src/server/server_utils.c
        src/logging.c
        src/commands.c
        src/server/hash_table.c
        src/send_message.c
        src/launch_arguments.c
        src/receive_message.c
        src/web_server.c
        lib/mongoose.c
        lib/cJSON.c
)

add_executable(client # Define the client build
        src/client/main.c
        src/server/server_utils.c
        src/client/client_errors.c
        src/client/client_utils.c
        src/receive_message.c
        src/send_message.c
        src/logging.c
        src/commands.c
        src/launch_arguments.c
        src/server/hash_table.c # Include hash_table.c for the client
)

add_executable(test_hash_table # Define the test hash table
        tests/test_hash_table.c
        src/server/hash_table.c  
        src/logging.c  
)
target_link_libraries(test_hash_table PRIVATE cmocka)
target_include_directories(test_hash_table PRIVATE include)

add_executable(test_commands # Define the tests for commands
        tests/test_commands.c
        src/commands.c
        src/logging.c
        src/send_message.c
)
target_link_libraries(test_commands PRIVATE cmocka)
target_include_directories(test_commands PRIVATE include)

add_executable(test_webserver # Define the tests for the webserver
        tests/test_webserver.c
        src/web_server.c
        src/logging.c
        lib/mongoose.c
        src/server/hash_table.c
        src/commands.c
        src/send_message.c
        src/receive_message.c
        src/server/server_utils.c
        lib/cJSON.c
)
target_link_libraries(test_webserver PRIVATE cmocka)
target_include_directories(test_webserver PRIVATE include)

# Add the test target
add_executable(test_receive_message
    tests/test_receive_message.c
    src/receive_message.c
    src/logging.c
    src/server/hash_table.c
    src/server/server_utils.c
)
target_link_libraries(test_receive_message PRIVATE cmocka)
target_include_directories(test_receive_message PRIVATE include)
target_compile_definitions(test_receive_message PRIVATE CLIENT_MESSAGES_DIR="test_client_messages")
