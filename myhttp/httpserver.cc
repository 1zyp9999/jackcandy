#include "httpserver.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <thread>
#include <fstream>
#include <vector>
#include <string>

HttpServer::HttpServer(int port, size_t nums_threads) : port_(port), server_socket_(-1), thread_pool_(nums_threads) {}

HttpServer::~HttpServer() {
    if (server_socket_ >= 0) {
        close(server_socket_);
    }
}

void HttpServer::init_Server() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1) {
        std::cerr << "socket creation failed!" << std::endl;
        exit(1);
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "bind failed: " << strerror(errno) << std::endl;
        close(server_socket_);
        exit(1);
    }

    if (listen(server_socket_, 10) == -1) {  // 确保调用 listen() 使得服务器准备接收连接
        std::cerr << "listen failed: " << strerror(errno) << std::endl;
        close(server_socket_);
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "setsockopt failed: " << strerror(errno) << std::endl;
        close(server_socket_);
        exit(1);
    }

    std::cout << "server listening on port " << port_ << "...\n";
}

void HttpServer::handle_request(int client_socket) {
    char buffer[1024];
    int received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        std::cerr << "Error receiving data.\n";
        return;
    }

    std::string request(buffer, received);
    std::string method, path;
    parse_request(request, method, path);  // 解析方法和路径,没有提取 HTTP 版本

    std::map<std::string, std::string> headers; // 存储请求头
    parse_headers(request, headers); // 解析请求头

    // 打印请求头信息（调试用）
    for (const auto& header : headers) {
        std::cout << header.first << ": " << header.second << std::endl;
    }

    if (method == "GET") {
        std::string body;
        if (path == "/") {
            body = "<html><body><h1>zhangyupeng</h1></body></html>";
            send_request(client_socket, body, "text/html");
        } else {
            std::ifstream file("." + path, std::ios::binary);
            if (file.is_open()) {
                std::ostringstream oss;
                oss << file.rdbuf();
                body = oss.str();
                send_request(client_socket, body, "text/html");
            } else {
                body = "<html><body><h1>404 Not Found</h1></body></html>";
                send_request(client_socket, body, "text/html");
            }
        }
    }
    if (method == "POST") {
        // 找到请求体开始位置
        size_t body_start = request.find("\r\n\r\n") + 4;
        std::string post_body = request.substr(body_start);

        // 简单处理application/x-www-form-urlencoded
        std::vector<std::string> key_value_pairs;
        size_t pos = 0;
        while ((pos = post_body.find('&')) != std::string::npos) {
            key_value_pairs.push_back(post_body.substr(0, pos));
            post_body.erase(0, pos + 1);
        }
        key_value_pairs.push_back(post_body);

        std::string response_body = "<html><body><h1>POST Data Received</h1><ul>";
        for (const auto& pair : key_value_pairs) {
            response_body += "<li>" + pair + "</li>";
        }
        response_body += "</ul></body></html>";
        std::cout << "this is post" << std::endl;

        send_request(client_socket, response_body, "text/html");
    }

    close(client_socket);
}

void HttpServer::send_request(int client_socket, const std::string& body, const std::string& content_type) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;

    send(client_socket, response.str().c_str(), response.str().length(), 0);
}

void HttpServer::parse_request(const std::string& request, std::string& method, std::string& path) {
    std::istringstream request_stream(request);
    request_stream >> method >> path;
}

void HttpServer::parse_headers(const std::string& request, std::map<std::string, std::string>& headers) {
    std::istringstream request_stream(request);
    std::string line;

    // 跳过请求的第一行（方法和路径）
    std::getline(request_stream, line);

    // 解析剩余的请求头部
    while (std::getline(request_stream, line) && !line.empty()) {
        size_t pos = line.find(":");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            key = trim(key);
            value = trim(value);
            headers[key] = value;
        }
    }
}

std::string HttpServer::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    size_t last = str.find_last_not_of(" \t\r\n");
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

void HttpServer::start() {
    init_Server();

    // 创建 epoll 实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Failed to create epoll" << std::endl;
        exit(1);
    }

    // 设置 server_socket 为非阻塞
    fcntl(server_socket_, F_SETFL, O_NONBLOCK);

    // 注册 server_socket 监听事件
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;  // Edge-triggered
    event.data.fd = server_socket_;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_, &event) == -1) {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        exit(1);
    }

    struct epoll_event events[10];  // 最大支持10个事件

    while (true) {
        // 等待事件
        int nfds = epoll_wait(epoll_fd, events, 10, -1);
        if (nfds == -1) {
            std::cerr << "epoll_wait failed" << std::endl;
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_socket_) {
                // 新连接
                int client_socket = accept(server_socket_, nullptr, nullptr);
                if (client_socket == -1) {
                    std::cerr << "Failed to accept client" << std::endl;
                    continue;
                }

                // 设置 client_socket 为非阻塞
                fcntl(client_socket, F_SETFL, O_NONBLOCK);

                // 将 client_socket 添加到 epoll 中
                event.events = EPOLLIN | EPOLLET;  // Edge-triggered
                event.data.fd = client_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    std::cerr << "Failed to add client socket to epoll" << std::endl;
                    close(client_socket);
                }
            } else {
                // 处理已就绪的客户端请求
                int client_socket = events[i].data.fd;
                thread_pool_.enqueue([this, client_socket] {
                    handle_request(client_socket);
                });
            }
        }
    }

    close(epoll_fd);
}
