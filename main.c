#include <stdio.h>      // for printing
#include <unistd.h>     // for syscalls
#include <stdlib.h>     // for malloc
#include <sys/socket.h> // for socket
#include <string.h>     // for memset
#include <netdb.h>      // for addrinfo
#include <errno.h>      // for erno

#define PORT "8080"
#define BACKLOG 3

const char notfound_message[] = "HTTP/1.1 404 Not Found\r\nContent-Type:text/plain\r\n\r\nNot Found";
const char ok_message[] = "HTTP/1.1 200 OK\r\nContent-Type:";

char * gnu_getcwd () {
    size_t size = 100;
    while (1)
    {
        char *buffer = (char *) malloc (size);
        if (getcwd (buffer, size) == buffer)
            return buffer;
        free(buffer);
        if (errno != ERANGE)
            return 0;
        size *= 2;
    }
}

char* get_mime_type(char* file_extension) {
    if (file_extension == NULL) {
        return "text/plain";
    } else if (!strcmp(file_extension, ".html")) {
        return "text/html";
    } else if (!strcmp(file_extension, ".css")) {
        return "text/css";
    } else if (!strcmp(file_extension, ".js")) {
        return "application/javascript";
    }
    return "text/plain";
}

void accept_client(int peer_fd)
{
    FILE *peer_stream = fdopen(peer_fd, "r+"); // convert to file to enable getline

    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, peer_stream);
    printf("%s", line);

    char *saveptr;
    char *method = strtok_r(line, " ", &saveptr);
    char *path = strtok_r(NULL, " ", &saveptr);
    for (char *token = strtok_r(NULL, " ", &saveptr); token != NULL; token = strtok_r(NULL, " ", &saveptr));
    if (!strcmp(method, "GET")) {
        // append cwd to path
        char *cwd = gnu_getcwd();
        size_t cwd_length = strlen(cwd);
        cwd = (char *)realloc(cwd, cwd_length + strlen(path));
        strcpy(cwd + cwd_length, path);
        // read file
        printf("Reading %s\n", cwd);
        FILE *file = fopen(cwd, "r");
        if (file == NULL) {
            write(peer_fd, notfound_message, sizeof(notfound_message) - 1);
        } else {
            write(peer_fd, ok_message, sizeof(ok_message) - 1);
            // find mime type
            char* mime_type = get_mime_type(strstr(cwd, "."));
            write(peer_fd, mime_type, strlen(mime_type));
            write(peer_fd, "\r\n\r\n", 4);
            char buff[2048];
            size_t bytes_read = fread(buff, sizeof(char), sizeof(buff), file);
            while (bytes_read != 0) {
                write(peer_fd, buff, bytes_read);
                bytes_read = fread(buff, sizeof(char), sizeof(buff), file);
            }
            fclose(file);
        }
        free(cwd);
    }
    free((void *)line); // also makes method and path invalid
    // close connection
    fclose(peer_stream);
}

int main()
{
    // create and bind socket
    struct addrinfo hints, *res;
    // get address by hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, PORT, &hints, &res)) {
        fprintf(stderr, "Can't get address %s.", PORT);
        exit(1);
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    // reuse address
    int i_set_option = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&i_set_option, sizeof(i_set_option));
    // bind to address
    if (bind(sock_fd, res->ai_addr, res->ai_addrlen)) {
        fprintf(stderr, "Can't bind to %s", PORT);
        exit(1);
    }

    // listen for incoming connections
    listen(sock_fd, BACKLOG);
    
    // accept loop
    while (1)
    {
        struct sockaddr_storage peer_address;
        socklen_t peer_address_len = sizeof peer_address;
        int peer_fd = accept(sock_fd, (struct sockaddr *)&peer_address, &peer_address_len);
        accept_client(peer_fd);
    }

    return 0;
}
