#pragma once

#include <sys/types.h>

#define MEDIA_GIF "image/gif"
#define MEDIA_JPEG "image/jpeg"
#define MEDIA_PNG "image/png"
#define MEDIA_HTML "text/html; charset=utf-8"
#define MEDIA_CSS "text/css"
#define MEDIA_JAVASCRIPT "text/javascript"
#define MEDIA_OCTET_STREAM "application/octet-stream"
#define MEDIA_ICO "image/x-icon"

void log_error(const char* errMsg);
void log_error_and_errno(const char* errMsg);

int get_file_size(int fd);
void get_file_extension(const char* filePath, char* bfrExt, size_t bfrSize);
void get_mediatype(const char* ext, char* bfrMedia, size_t bfrSize);
int is_mediatype_image(const char* mediaType);

int contains_relative_paths(const char* path);
int decode_uri(const char* encodedUri, char* decodedUri, size_t bufferSize);
