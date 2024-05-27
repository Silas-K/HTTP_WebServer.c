#pragma once

#include "server.h"
#include <sys/types.h>

#define NUM_MATCHES 4

void read_request(int cs, char* request, size_t bfrSize);
int parse_request(const char* request, char* method, char* uri, char* version);
int send_file_buffered(int cs, int fd);
int send_header(int cs, int statusCode, const char* reasonPhrase, char headerFields[][BUFSIZE_SMALL], int hFieldCount);
