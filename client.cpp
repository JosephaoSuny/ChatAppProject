#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>

#include "shared.h"

#define VERY_LONG_MESSAGE "This message is very long. The goal here is to be well over 100 characters in order to test the read chunking system, but I am not sure what 100 characters looks like, so here we are."

int main() {
    auto socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{ .sin_family = AF_INET, .sin_port = PORT_VALUE, .sin_addr = { .s_addr = inet_addr("127.0.0.1") }, .sin_zero = { } };

    int result = connect(socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) ? errno : 0;

    int set_nb_res = fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL) | O_NONBLOCK);

    if (set_nb_res < 0) {
        exit(1000);
    }

    if (result != 0) {
        printf("%d\n", result);
        exit(100);
    }

    int counter = 0;

    send_message(socket_fd, VERY_LONG_MESSAGE, strlen(VERY_LONG_MESSAGE));
    Message message { };

    bool has_response = false;

    while (true) {
        message.reset();
        const long status = read_message(socket_fd, &message) < 0 ? errno : 0;
        if (status == EWOULDBLOCK) { }
        else if (status == 0) {
            if (message.length == 0) {
                counter++;
                if (counter == 100) break;
                continue;
            }

            printf("Got message %s\n", message.buffer);

            if (strcmp(message.buffer, ":q") == 0) {
                send_message(socket_fd, ":q", 2);
                printf("Exiting...\n");
                break;
            }

            has_response = true;
        } else {
            printf("Error: %ld\n", status);
        }

        if (has_response) {
            std::string s;
            std::getline(std::cin, s);

            if (s.empty()) continue;

            std::string outgoing = "user 2: " + s;

            long send_result = send_message(socket_fd, outgoing.c_str(), outgoing.length());

            printf("send result: %ld\n", send_result);

            if (send_result < 0) printf("%d\n", errno);
            has_response = false;
        }
    }

    close(socket_fd);
}
