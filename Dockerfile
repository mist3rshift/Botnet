# Base image
FROM ubuntu:latest

# Set environment variables to avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install required packages
RUN apt-get update -y && apt-get install -y \
    build-essential \
    cmake \
    clang \
    git \
    libprotobuf-dev \
    protobuf-compiler \
    && apt-get clean

# Set the working directory inside the container
WORKDIR /app

# Copy the project files into the container
COPY . .

# Run CMake to configure and build the project
RUN cmake -S . -B build
RUN cmake --build build

# Default command to run when the container starts
CMD ["./build/test_commands"]

