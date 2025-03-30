cmake_minimum_required(VERSION 3.30)
project(botnet)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(server # Define the server build
        src/server/main.c
        src/server/server_errors.c
        src/logging.c
)

add_executable(client # Define the client build
        src/client/main.c
        src/client/client_errors.c
        src/logging.c
)