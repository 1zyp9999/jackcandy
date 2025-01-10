#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <map>
#include "thread.h"

class HttpServer {
public:
    HttpServer(int port,size_t num_threads);
    ~HttpServer();

    void start();

private:
    int port_;
    int server_socket_;
    ThreadPool thread_pool_;

    void init_Server();
    void handle_request(int client_socket);
    void send_request(int client_socket, const std::string& body, const std::string& content_type);
    void parse_request(const std::string& request, std::string& method, std::string& path);
    std::string trim(const std::string& str);
    void parse_headers(const std::string& request, std::map<std::string, std::string>& headers);

};

#endif // HTTP_SERVER_H
