#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>

class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();

    void start();

private:
    int port_;
    int server_socket_;

    void init_Server();
    void handle_request(int client_socket);
    void send_request(int client_socket, const std::string& body, const std::string& content_type);
    void parse_request(const std::string& request, std::string& method, std::string& path);
};

#endif // HTTP_SERVER_H
