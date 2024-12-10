#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <unistd.h>
#include <vector>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <queue>
#include <shared.h>


namespace ImGui {
    IMGUI_API bool  InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
}

int accept_timeout(int server_socket, int seconds_timeout, sockaddr_in * new_addr, socklen_t size);

std::atomic_bool shouldListen = true;

int main() {
    // Create a local host addr
    sockaddr_in addr{ .sin_family = AF_INET, .sin_port = PORT_VALUE, .sin_addr = { .s_addr = inet_addr("127.0.0.1") }, .sin_zero = { } };
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

    const int connection_fd = accept_timeout(server_socket, 10, &new_addr, sizeof(new_addr));

    if (connection_fd < 0) return 3;

    if (const int set_nb_res = fcntl(connection_fd, F_SETFL, fcntl(connection_fd, F_GETFL) | O_NONBLOCK); set_nb_res < 0) return 3;

    if (!glfwInit()) return 1;

    auto glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::SetNextWindowSize({1280, 720});
    ImGui::SetNextWindowPos({0, 0});

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    constexpr auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    Message message { };

    bool connection_is_closed = false;

    std::vector<std::string> history = {};
    std::string buffer = { };
    while (!glfwWindowShouldClose(window)) {
        if (!connection_is_closed) {
            const long result = read_message(connection_fd, &message);
            const long status = result < 0 ? errno : 0;

            if (status == 0 && message.buffer != nullptr) {
                std::string client_message = { message.buffer, message.length };
                std::cout << client_message << "\n";
                history.push_back(client_message);
                if (client_message == ":q") {
                    const auto close_message = ":q";

                    const long message_result = send_message(connection_fd, close_message, strlen(close_message));

                    printf("result: %ld, errno: %d \n", message_result, errno);
                    glfwSetWindowShouldClose(window, true);
                    connection_is_closed = true;
                }
            } else if (status == EWOULDBLOCK) {}
            else printf("%ld\n", status);
        }

        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Message App", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        auto outer_size = ImVec2(0.0f, 720 - (ImGui::GetFontSize() + ImGui::GetFrameHeight()*2));

        if (ImGui::BeginTable("Table ID", 1, ImGuiTableFlags_ScrollY, outer_size)) {
            for (const auto & text : history) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                try {
                    ImGui::TextWrapped("%s", text.c_str());
                } catch (std::out_of_range& e) {
                    std::cout << e.what() << "\n";
                }
            }
            ImGui::EndTable();
        }

        ImGui::Text("Message");
        ImGui::SameLine();
        if (ImGui::InputText("##TextInput", &buffer, ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::cout << buffer << "\n";
            send_message(connection_fd, buffer.c_str(), buffer.length());
            history.push_back("user 1: " + buffer);
            buffer.clear();
        }

        // Rendering
        ImGui::End();
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    close(server_socket);

    return 0;
}

int accept_timeout(const int server_socket, const int seconds_timeout, sockaddr_in* new_addr, socklen_t size) {
    timeval timeout { .tv_sec = seconds_timeout, .tv_usec = 0 };
    fd_set server_socket_fd_set { };

    FD_SET(server_socket, &server_socket_fd_set);

    const int result = select(server_socket + 1, &server_socket_fd_set, nullptr, nullptr, &timeout);

    if (result > 0) return accept(server_socket, reinterpret_cast<sockaddr*>(new_addr), &size);

    return -1;
}