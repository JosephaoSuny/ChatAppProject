#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <thread>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>

#include "shared.h"

char* message();

int accept_timeout(int, int, sockaddr_in*, socklen_t);

int main() {
    // Create a local host addr
    sockaddr_in addr{ .sin_family = AF_INET, .sin_port = 4500, .sin_addr = { .s_addr = inet_addr("127.0.0.1") } };
    const auto server_socket = socket(AF_INET, SOCK_STREAM, 0);
    const int response = bind(server_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    if (response < 0) {
        printf("%d %d\n", response, errno);
    }

    const int listen_response = listen(server_socket, 4);

    if (listen_response < 0) {
        printf("%d\n", listen_response);
    }

    sockaddr_in new_addr {};
    constexpr socklen_t addr_size = sizeof(new_addr);

    const int connection_fd = accept_timeout(server_socket, 10, &new_addr, addr_size);

    if (connection_fd < 0) return 2;

    Message message { };
    long message_result = read_message(connection_fd, &message);

    if (message_result < 0) {
        return 3;
    }

    printf("result: %ld, errno: %d, msg: %s\n", message_result, errno, message.buffer);

    message_result = send_message(connection_fd, "Hello, Client!", strlen("Hello, Client!"));

    printf("result: %ld, errno: %d \n", message_result, errno);

    while (true) {
        message = { };
        long result = read_message(connection_fd, &message);
        long status = result < 0 ? errno : 0;

        if (status == EWOULDBLOCK) {}
        else printf("%ld\n", status);

        if (message.buffer != nullptr) {
            printf("%s\n", message.buffer);
            if (strcmp(message.buffer, ":q") == 0) {
                const auto close_message = ":q";

                message_result = send_message(connection_fd, close_message, strlen(close_message));

                printf("result: %ld, errno: %d \n", message_result, errno);
                break;
            }
        }

        const std::string placeholder = "Here we would normally take a second input, but it's easier to auto reply";

        send_message(connection_fd, placeholder.c_str(), placeholder.length());
    }

    int status = close(server_socket);

    if (status < 0) {
        const int error = errno;

        printf("error: %d", error);
        return error;
    }

    return 0;
}

int accept_timeout(const int server_socket, const int seconds_timeout, sockaddr_in* new_addr, socklen_t size) {
    timeval timeout { .tv_sec = seconds_timeout };
    fd_set server_socket_fd_set { };

    FD_SET(server_socket, &server_socket_fd_set);

    const int result = select(server_socket + 1, &server_socket_fd_set, nullptr, nullptr, &timeout);

    if (result > 0) return accept(server_socket, reinterpret_cast<sockaddr*>(new_addr), &size);

    return -1;
}

char* message() {
    constexpr int MAX_MESSAGE = 1024;

    const auto buffer = static_cast<char*>(malloc(MAX_MESSAGE));

    scanf("%c", buffer);

    return buffer;
}
