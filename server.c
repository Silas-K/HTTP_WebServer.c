#include <fcntl.h>        // open
#include <netinet/in.h>   // sockaddr_in, htons
#include <signal.h>       // sigaction, SIGINT
#include <stdio.h>        // printf
#include <stdlib.h>       // exit
#include <string.h>       // strerror
#include <strings.h>      // bzero, strncmp
#include <sys/sendfile.h> // sendfile
#include <sys/socket.h>   // accept, bind, listen
#include <sys/wait.h>     // waitpid
#include <unistd.h>       // read, close, fork, exit

#include "helper.h"
#include "server.h"
#include "server_io.h"

int server_socket = -1;
int client_socket = -1;

int main(void)
{
    setup_signal_handlers();

    int addrlen;
    struct sockaddr_in client_addr;
    printf("Starting webserver...\n");
    server_socket = setup_webserver();

    // endless server loop
    while (1)
    {
        // clear all terminated child processes, but dont hang
        waitpid(-1, NULL, WNOHANG);

        addrlen = sizeof(client_addr);

        // wait for client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addrlen);
        if (client_socket == -1)
        {
            log_error_and_errno("Error accepting the client socket");
            continue;
        }

        // handle request
        int id = fork();
        if (id == -1)
        {
            // error
            log_error_and_errno("Error forking");
            close(client_socket);
        }
        else if (id == 0)
        {
            // child process
            setup_signal_handlers();
            close(server_socket);
            printf("---- Handling Request  ----\n\n");
            int result = handle_request(client_socket);
            write(client_socket, "\0", 1);
            close(client_socket);
            printf("---- Connection closed ----\n\n\n");
            exit(result);
        }
        else
        {
            // parent process
            close(client_socket);
        }
    }
}

/** Signal handler for SIGINT (Ctrl+C)
 * @param signal: The signal number.
 */
void handle_signal(int signal)
{
    if (signal == SIGINT)
    {
        printf("\nCaught signal %d, closing server socket...\n", signal);
        if (server_socket != -1)
            close(server_socket);
        if (client_socket != -1)
            close(client_socket);

        exit(EXIT_SUCCESS);
    }
}

/** Setup signal handlers for SIGINT (Ctrl+C)
 *
 */
void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        log_error_and_errno("Error setting up signal handler");
        exit(EXIT_FAILURE);
    }
}

/**
 * Setup the webserver on localhost port 80.
 * @return The socket descriptor of the server socket.
 * */
int setup_webserver()
{
    int s, ret;
    struct sockaddr_in s_addr;

    // create TCP/IP socket
    s = socket(AF_INET, SOCK_STREAM, 0);

    // localhost port 80 (http)
    bzero(&s_addr, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(80);
    s_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket to address
    ret = bind(s, (struct sockaddr*)&s_addr, sizeof(s_addr));
    if (ret < 0)
    {
        log_error_and_errno("Bind Error");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    listen(s, 5);

    return (s);
}

/**
 * Handles a request from a client.
 * @param cs: The client socket to handle the request for.
 * @return 0 if the request was successfully handled, 1 otherwise.
 * */
int handle_request(int cs)
{
    char request[BUFSIZE];
    char method[BUFSIZE_SMALL], eUri[BUFSIZE_SMALL], uri[BUFSIZE_SMALL], version[BUFSIZE_SMALL],
        fileExtension[BUFSIZE_SMALL], mediaType[BUFSIZE_SMALL];
    char* filePath;

    printf("Getting request: ");
    read_request(cs, request, sizeof(request));
    printf("%s\n", request);

    int result = parse_request(request, method, eUri, version);

    // Parse request with regex
    // "GET /index.html HTTP/1.1"
    if (result != 0)
    {
        log_error("Error parsing request with regex");
        send_header(cs, 400, "Bad Request", NULL, 0);
        return 1;
    }

    // check if method is GET
    if (strcmp(method, "GET") != 0)
    {
        log_error("Method not supported");
        send_header(cs, 501, "Not implemented", NULL, 0);
        return 1;
    }

    // decode uri, e.g. "%20" -> " " (space)
    result = decode_uri(eUri, uri, sizeof(uri));
    if (result != 0)
    {
        log_error("Error decoding uri");
        send_header(cs, 400, "Bad Request", NULL, 0);
        return 1;
    }

    // dont allow relative paths ("..", "~")
    if (contains_relative_paths(uri))
    {
        log_error("Invalid Path component found (\"..\" | \"~\")");
        send_header(cs, 403, "Forbidden", NULL, 0);
        return 1;
    }

    filePath = uri;
    // map "*/" to "*/index.html"
    // check if last character is "/", then append "index.html"
    if (strcmp(filePath + strlen(filePath) - 1, "/") == 0)
    {
        strcat(filePath, DEFAULT_PATH);
    }

    // trim leading "/"
    if (strncmp(filePath, "/", 1) == 0)
    {
        filePath = filePath + 1;
    }

    // get extension and mimetype
    get_file_extension(uri, fileExtension, sizeof(fileExtension));
    printf("FileExtension: %s\n", fileExtension);
    get_mediatype(fileExtension, mediaType, sizeof(mediaType));
    printf("MediaType: %s\n", mediaType);

    if (strlen(mediaType) == 0)
    {
        log_error("MediaType not supported");
        send_header(cs, 415, "Unsupported Media Type", NULL, 0);
        return 1;
    }

    printf("Method: %s\n", method);
    printf("URI: %s\n", uri);
    printf("Version: %s\n", version);
    printf("\n\n");

    // open file
    int fd = open(filePath, O_RDONLY);
    if (fd == -1)
    {
        log_error_and_errno("Error opening requested file");
        send_header(cs, 404, "Not Found", NULL, 0);
        return 1;
    }

    int fileSize = get_file_size(fd);

    // send header
    char fields[2][BUFSIZE_SMALL];
    snprintf(fields[0], BUFSIZE_SMALL, "Content-Length: %d", fileSize);
    snprintf(fields[1], BUFSIZE_SMALL, "Content-Type: %s", mediaType);
    result = send_header(cs, 200, "OK", fields, 2);
    if (result == -1)
    {
        log_error("Error sending the header");
        return 1;
    }

    // send body
    if (is_mediatype_image(mediaType))
    {
        // result = send_header(cs, 200, "OK", fields, 2);
        result = sendfile(cs, fd, NULL, fileSize);
        if (result == -1 || result != fileSize)
        {
            log_error_and_errno("Error sending file");
            return 1;
        }
    }
    else
    {
        result = send_file_buffered(cs, fd);
        if (result == -1)
        {
            log_error("Error sending file buffered");
            return 1;
        }
    }

    close(fd);
    return 0;
}
