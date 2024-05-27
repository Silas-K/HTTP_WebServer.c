#pragma once

#define BUFSIZE 8096       // Buffer for large messages
#define BUFSIZE_SMALL 1024 // Buffer for small messages
#define SERVER_NAME "MinimalHttpTestServer/0.0.1"
#define HTTP_VERSION "HTTP/1.1"
#define DEFAULT_PATH "index.html"

int handle_request(int clientSocket);
int setup_webserver();
void handle_signal(int signal);
void setup_signal_handlers();
