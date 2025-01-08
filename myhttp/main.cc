#include "httpserver.h"

int main() {
    HttpServer server(8081);
    server.start();
    return 0;
}
