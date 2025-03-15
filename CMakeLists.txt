cmake_minimum_required(VERSION 3.30)
project(botnet)

set(CMAKE_CXX_STANDARD 14)

add_executable(server # Define the server build
        src/server/main.c
        src/server/server_errors.c
        include/server/server_constants.h
        src/logging.c
)

add_executable(client # Define the client build
        src/client/main.c
        src/client/client_errors.c
        include/client/client_constants.h
        src/logging.c
)