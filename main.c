#include <stdio.h>      // for printing
#include <unistd.h>     // for syscalls
#include <stdlib.h>     // for malloc
#include <sys/socket.h> // for socket
#include <string.h>     // for memset
#include <netdb.h>      // for addrinfo

#define PORT "8080"
#define BACKLOG 3

void accept_client(int sock_fd)
{
    struct sockaddr_storage peer_address;
    socklen_t peer_address_len = sizeof peer_address;
    int peer_fd = accept(sock_fd, (struct sockaddr *)&peer_address, &peer_address_len);

    FILE *peer_stream = fdopen(peer_fd, "r+");

    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, peer_stream);

    char *saveptr;
    for (char *token = strtok_r(line, " ", &saveptr); token != NULL; token = strtok_r(NULL, " ", &saveptr))
    {
        printf("%s\n", token);
    }

    free((void *)line);

    // close connection
    close(peer_fd);
}

int main()
{
    // create and bind socket
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, PORT, &hints, &res)) {
        fprintf(stderr, "Can't get address %s.", PORT);
        exit(1);
    }

    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (bind(sock_fd, res->ai_addr, res->ai_addrlen)) {
        fprintf(stderr, "Can't bind to %s", PORT);
        exit(1);
    }

    // listen for incoming connections
    listen(sock_fd, BACKLOG);

    // accept loop
    while (1)
    {
        accept_client(sock_fd);
    }

    return 0;
}
