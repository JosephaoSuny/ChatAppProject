#include "shared.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>

Message::~Message() {
    if (buffer != nullptr) {
        if (capacity > 0) free(buffer);
    }
}

/// Zeros the buffer, and resets the length to zero, this prevents reading half messages/garbage data.
void Message::reset() {
    memset(buffer, 0, capacity);
    length = 0;
}

template<typename N>
N max(N a, N b) {
    if (a > b) return a;
    return b;
}

#define MAX_READ size_t(100)

/**
 * Reads a message from the connection.
 * **/
long read_message(const int connection_fd, Message* message) {
    // Receive the length of the message
    long result = recv(connection_fd, &message->length, sizeof(message->length), 0);
    if (result < 0 ) {
        return result;
    }

    if (message->buffer == nullptr) {
        message->buffer = static_cast<char*>(malloc(message->length));
    } else if (message->capacity < message->length) {
        const auto new_buffer = static_cast<char*>(realloc(message->buffer, message->length));
        if (new_buffer == nullptr) return -1;

        message->buffer = new_buffer;
    }

    // Allocate a buffer to hold the message
    message->capacity = max(message->length, message->capacity);

    // Store the total read bytes
    long read = 0;

    // While the read bytes doesn't equal the length, continue reading
    while (read != message->length) {
        // Read bytes left
        result = recv(connection_fd, message->buffer + read, MAX_READ, 0);
        // If the read errors, check it
        if (result < 0) {
            // if it returns `EWOULDBLOCK`, continue instead
            if (errno == EWOULDBLOCK) continue;

            // Otherwise, loop, invalidate the buffer, and return -1 so the caller can check the `errno`
            printf("oops\n");
            message->reset();
            return -1;
        }

        read += result;
    }

    return read;
}

long send_message(const int connection_fd, const char* message, const size_t length) {
    long result = send(connection_fd, &length, sizeof(length), 0);
    if (result < 0) return result;
    result = send(connection_fd, message, length, 0);

    return result;
}
