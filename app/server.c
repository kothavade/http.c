#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int server_fd, client_addr_len;
    struct sockaddr_in client_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }

    // Since the tester restarts your program quite often, setting SO_REUSEADDR
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEADDR failed: %s \n", strerror(errno));
        return 1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(4221),
        .sin_addr = {htonl(INADDR_ANY)},
    };

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        printf("Listen failed: %s \n", strerror(errno));
        return 1;
    }

    printf("Waiting for a client to connect...\n");
    client_addr_len = sizeof(client_addr);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    printf("Client connected\n");

    char req[1024];
    read(client_fd, req, 1024);

    char *line, *llast;
    line = strtok_r(req, "\r\n", &llast);

    // Request
    char *method, *target, *version, *reqlast;
    method = strtok_r(line, " ", &reqlast);
    target = strtok_r(NULL, " ", &reqlast);
    version = strtok_r(NULL, " ", &reqlast);
    // printf("METHOD: %s\n", method);
    // printf("TARGET: %s\n", target);
    // printf("VERSION: %s\n", version);

    const char *OK = "HTTP/1.1 200 OK\r\n\r\n";
    const char *NOT_FOUND = "HTTP/1.1 404 Not Found\r\n\r\n";
    const char *ECHO_PREFIX =
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: ";

    const char *echo_target = "/echo/";

    if (strcmp(target, "/") == 0) {
        write(client_fd, OK, strlen(OK));
    } else if (strncmp(echo_target, target, strlen(echo_target)) == 0) {
        const char *input = target + strlen(echo_target);
        int input_len = strlen(input);
        char *echo = malloc(
            // prefix
            strlen(ECHO_PREFIX)
            // input-length length
            + snprintf(NULL, 0, "%d", input_len)
            // \r\n\r\n
            + strlen("\r\n\r\n")
            // input
            + strlen(input)
            // NULL
            + 1);
        sprintf(echo, "%s%d\r\n\r\n%s", ECHO_PREFIX, input_len, input);
        write(client_fd, echo, strlen(echo));
        free(echo);
    } else {
        write(client_fd, NOT_FOUND, strlen(NOT_FOUND));
    }

    close(server_fd);

    return 0;
}
