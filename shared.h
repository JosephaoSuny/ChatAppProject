#ifndef SHARED_H
#define SHARED_H

#define PORT_VALUE 8080

#include <cstddef>

/// In memory message from connected client
typedef struct Message {
    size_t length = 0;
    size_t capacity = 0;
    char* buffer = nullptr;

    ~Message();

    void reset();
} Message;

long send_message(int connection_fd, const char* message, size_t length);

long read_message(int connection_fd, Message* message);

void reset_message(Message* message);

#endif //SHARED_H
