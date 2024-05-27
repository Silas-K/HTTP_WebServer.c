#include <regex.h>  // regex comparisons
#include <stdio.h>  // printf
#include <string.h> // strerror
#include <unistd.h> // read, write

#include "server_io.h"

/**
 * Reads a request from the given client socket.
 * @param cs: The client socket to read from.
 * @param request: The buffer to store the request.
 * @param bfrSize: The size of the buffer.
 */
void read_request(int cs, char* request, size_t bfrSize)
{
    char c;
    int i = 0; // Puffer für ein eingelesenes Zeichen
    while (read(cs, &c, 1) == 1 && c != '\n' && c != '\r' && i < bfrSize - 1)
        request[i++] = c;

    request[i] = '\0'; // String terminieren
}

/**
 * Parses a http request into its components.
 * @param request: The request to parse.
 * @param method: The buffer to store the HTTP method.
 * @param uri: The buffer to store the request URI.
 * @param version: The buffer to store the HTTP version.
 * @return 0 if the request was successfully parsed, 1 otherwise.
 */
int parse_request(const char* request, char* method, char* uri, char* version)
{
    regex_t reg;
    regmatch_t matches[NUM_MATCHES];

    // capture groups
    // 1: HTTP method
    // 2: request URI
    // 3: HTTP-Version
    const char* pattern = "^([^ ]*)"                  // match first word without space -> method, e.g. GET
                          " "                         // match space
                          "([^ ]*)"                   // match request URI, only allow absolute path
                          " "                         // match space
                          "HTTP\\/([0-9]+\\.[0-9]+)"; // match HTTP-Version (HTTP/1*digit.1*digit), e.g. HTTP/1.1
    // compile regex
    int compResult = regcomp(&reg, pattern, REG_EXTENDED);
    if (compResult != 0)
    {
        printf("Error %d", compResult);
        return 1;
    }

    int result = regexec(&reg, request, NUM_MATCHES, matches, 0);
    regfree(&reg);

    if (result == REG_NOMATCH || result != 0)
    {
        return 1;
        printf("No matches");
    }

    // Ensure null termination for each component
    int len = matches[1].rm_eo - matches[1].rm_so;
    snprintf(method, len + 1, "%s", request + matches[1].rm_so);

    len = matches[2].rm_eo - matches[2].rm_so;
    snprintf(uri, len + 1, "%s", request + matches[2].rm_so);

    len = matches[3].rm_eo - matches[3].rm_so;
    snprintf(version, len + 1, "%s", request + matches[3].rm_so);

    return 0;
}

/**
 * Sends a file to the client socket.
 * @param cs: The client socket to send the file to.
 * @param fd: The file descriptor of the file to send.
 * @return 0 if the file was successfully sent, 1 otherwise.
 */
int send_file_buffered(int cs, int fd)
{
    char buffer[BUFSIZE];
    int bytesRead, bytesWritten;

    while ((bytesRead = read(fd, buffer, BUFSIZE)) > 0)
    {
        bytesWritten = write(cs, (void*)buffer, bytesRead);
        if (bytesWritten < 0)
            return 1;
    }

    if (bytesRead < 0)
        return 1;
    return 0;
}

/**
 * Sends a http header to the client socket.
 * @param cs: The client socket to send the header to.
 * @param statusCode: The status code of the header.
 * @param reasonPhrase: The reason phrase of the header.
 * @param headerFields: The header fields to append to the header.
 * @param numHeader: The number of header fields.
 * @return 0 if the header was successfully sent, 1 otherwise.
 */
int send_header(int cs, int statusCode, const char* reasonPhrase, char headerFields[][BUFSIZE_SMALL], int numHeader)
{
    char header[BUFSIZE];
    int total_len = 0;

    // apply generic header fields
    total_len += snprintf(header, BUFSIZE,
                          "%s %d %s\r\n"
                          "Server: %s\r\n",
                          HTTP_VERSION, statusCode, reasonPhrase, SERVER_NAME);

    // append custom header fields
    for (int i = 0; i < numHeader; i++)
    {
        total_len += snprintf(header + total_len, BUFSIZE - total_len, "%s\r\n", headerFields[i]);
    }
    // append newline
    total_len += snprintf(header + total_len, BUFSIZE - total_len, "\r\n");

    int result = write(cs, (void*)header, strlen(header));
    return (result == -1 || result < strlen(header)) ? -1 : 0;
}
