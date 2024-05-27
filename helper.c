#include <ctype.h>    // isxdigit
#include <errno.h>    // strerror?, errno
#include <stdio.h>    // printf
#include <stdlib.h>   // exit
#include <string.h>   // strerror
#include <sys/stat.h> // fstat

#include "helper.h"

/**
 * Logs an error message to stderr.
 * @param errMsg: The error message to log.
 * */
void log_error(const char* errMsg)
{
    fprintf(stderr, "%s\n", errMsg);
}

/**
 * Logs an error message to stderr and appends the error message of the current errno.
 * @param errMsg: The error message to log.
 * */
void log_error_and_errno(const char* errMsg)
{
    fprintf(stderr, "%s: %s\n", errMsg, strerror(errno));
}

/**
 * Get the file size of the given file descriptor.
 * @param fd: The file descriptor to get the size for.
 * */
int get_file_size(int fd)
{
    struct stat fileStats;
    fstat(fd, &fileStats);
    int fsize = fileStats.st_size;
    return fsize;
}

/**
 * Returns the file extension of the given file path.
 * @param filePath: The file path to get the extension for.
 * @param bfrExt: The buffer to store the file extension.
 * @param bfrSize: The size of the buffer.
 * */
void get_file_extension(const char* filePath, char* bfrExt, size_t bfrSize)
{
    char* dot = strrchr(filePath, '.');
    if (dot == NULL)
        bfrExt = NULL;
    else
        snprintf(bfrExt, bfrSize, "%s", dot + 1);
}

/**
 * Returns the media type for the given file extension.
 * @param ext: The file extension to get the media type for.
 * @param bfrMedia: The buffer to store the media type.
 * @param bfrSize: The size of the buffer.
 * */
void get_mediatype(const char* ext, char* bfrMedia, size_t bfrSize)
{
    if (strcmp(ext, "gif") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_GIF);
    else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_JPEG);
    else if (strcmp(ext, "png") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_PNG);
    else if (strcmp(ext, "htm") == 0 || strcmp(ext, "html") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_HTML);
    else if (strcmp(ext, "css") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_CSS);
    else if (strcmp(ext, "js") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_JAVASCRIPT);
    else if (strcmp(ext, "ico") == 0)
        snprintf(bfrMedia, bfrSize, MEDIA_ICO);
    else
        bfrMedia[0] = '\0';
}

/**
 * Checks if the given media type is an image type.
 * @param mediaType: The media type to check.
 * @return 1 if the media type is an image type, 0 otherwise;
 */
int is_mediatype_image(const char* mediaType)
{
    return strcmp(mediaType, MEDIA_GIF) == 0 || strcmp(mediaType, MEDIA_JPEG) == 0 || strcmp(mediaType, MEDIA_PNG) == 0;
}

/**
 * Checks if the given path contains relative paths or tilde characters.
 * @param path: The path to check.
 * @return 1 if the path contains relative paths or tilde characters, 0 otherwise.
 */
int contains_relative_paths(const char* path)
{
    return strstr(path, "..") != NULL || strchr(path, '~') != NULL;
}

/**
 * Decodes a URI string by replacing percent-encoded characters with their ASCII equivalents.
 * @param encodedUri: The URI string to decode.
 * @param decodedUri: The buffer to store the decoded URI string.
 * @param bufferSize: The size of the buffer.
 * @return 0 if the URI was successfully decoded, 1 otherwise.
 */
int decode_uri(const char* encodedUri, char* decodedUri, size_t bufferSize)
{
    if (encodedUri == NULL || decodedUri == NULL || bufferSize == 0)
        return 1;

    const char* pstr = encodedUri;
    char* pbuf = decodedUri;
    size_t remainingSize = bufferSize - 1; // Leave room for null terminator

    while (*pstr && remainingSize > 0)
    {
        if (*pstr == '%')
        {
            // Ensure there are at least two more characters after '%' and both are hex digits
            if (pstr[1] && pstr[2] && isxdigit((unsigned char)pstr[1]) && isxdigit((unsigned char)pstr[2]))
            {
                char hex[3];
                hex[0] = pstr[1];
                hex[1] = pstr[2];
                hex[2] = '\0';
                *pbuf++ = (char)strtol(hex, NULL, 16); // Convert hex to char
                pstr += 3;
                remainingSize--;
            }
            else
            {
                *pbuf++ = *pstr++; // Copy '%' as is if not a valid encoding
                remainingSize--;
            }
        }
        else if (*pstr == '+')
        {
            *pbuf++ = ' '; // Convert '+' to space
            pstr++;
            remainingSize--;
        }
        else
        {
            *pbuf++ = *pstr++; // Copy other characters as is
            remainingSize--;
        }
    }
    *pbuf = '\0'; // Null-terminate the decoded string

    if (*pstr && remainingSize == 0)
        return 1;

    return 0; // Return success status
}
