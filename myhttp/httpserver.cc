#include "httpserver.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

HttpServer::HttpServer(int port) : port_(port), server_socket_(-1) {}

HttpServer::~HttpServer()
{
    if (server_socket_ >= 0)
    {
        close(server_socket_);
    }
}

void HttpServer::init_Server()
{

    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == -1)
    {
        std::cerr << "socket creation failed!" << std::endl;
        exit(1);
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        std::cerr << "bind failed: " << strerror(errno) << std::endl;
        close(server_socket_);
        exit(1);
    }

    if (listen(server_socket_, 10) == -1)  // 确保调用 listen() 使得服务器准备接收连接
    {
        std::cerr << "listen failed: " << strerror(errno) << std::endl;
        close(server_socket_);
        exit(1);
    }
    int opt = 1;
if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
{
    std::cerr << "setsockopt failed: " << strerror(errno) << std::endl;
    close(server_socket_);
    exit(1);
}


    std::cout << "server listening on port " << port_ << "...\n";


}

void HttpServer::handle_request(int client_socket)
{
    char buffer[1024];
    int received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (received <= 0)
    {
        std::cerr << "Error receiving data.\n";
        return;
    }

    std::string request(buffer, received);
    std::string method, path;
    parse_request(request, method, path);

    if (method == "GET")
    {
        std::string body;
        if (path == "/")
        {
            body = "<html><body><h1>there is my page!</h1></body></html>";
            send_request(client_socket, body, "text/html");
        }
        else
        {
            body = "<html><body><h1>404 Not Found</h1></body></html>";
            send_request(client_socket, body, "text/html");
        }
    }

    close(client_socket);
}

void HttpServer::send_request(int client_socket, const std::string& body, const std::string& content_type)
{
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;

    send(client_socket, response.str().c_str(), response.str().length(), 0);
}

void HttpServer::parse_request(const std::string& request, std::string& method, std::string& path)
{
    std::istringstream request_stream(request);
    request_stream >> method >> path;
}

void HttpServer::start()
{
    init_Server();

    while (true)
    {
        int client_socket = accept(server_socket_, nullptr, nullptr);
        if (client_socket == -1)
        {
            std::cerr << "accept failed" << std::endl;
            continue;
        }
        handle_request(client_socket);
    }
}
