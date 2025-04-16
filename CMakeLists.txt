cmake_minimum_required(VERSION 3.22)
project(botnet)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(server # Define the server build
        src/server/main.c
        src/server/server_errors.c
        src/logging.c
        src/commands.c
        src/server/hash_table.c
        src/send_message.c
        src/launch_arguments.c
        src/receive_message.c
        src/web_server.c
        lib/mongoose.c
)

add_executable(client # Define the client build
        src/client/main.c
        src/client/client_errors.c
        src/client/client_utils.c
        src/send_message.c
        src/logging.c
        src/commands.c
        src/launch_arguments.c
)

add_executable(test_hash_table # Define the test hash table
        tests/test_hash_table.c
        src/server/hash_table.c  
        src/logging.c  
)

add_executable(test_commands # Define the tests for commands
        tests/test_commands.c
        src/commands.c
        src/logging.c
)

add_executable(test_webserver # Define the tests for the webserver
        tests/test_webserver.c
        src/web_server.c
        src/logging.c
        lib/mongoose.c
        src/server/hash_table.c
        src/commands.c
)