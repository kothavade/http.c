#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "request.h"

#define HTTP_OK "HTTP/1.1 200 OK\r\n"

const char ECHO_TARGET[] = "/echo/";
const char USER_TARGET[] = "/user-agent";
const char FILE_TARGET[] = "/files/";

const char DIR_ARG[] = "--directory";

void *handleClient(void *arg);
void ok(Request *req);
void echo(Request *req);
void userAgent(Request *req);
void file(Request *req);
void notFound(Request *req);

char *dir = "./";

int main(int argc, char **argv) {
    if ((argc >= 2) && (strncmp(argv[1], DIR_ARG, strlen(DIR_ARG)) == 0)) {
        dir = argv[2];
    }
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
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
    while (true) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);

        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
        printf("Client connected\n");

        pthread_t thread;
        int *thread_fd = malloc(sizeof(int));
        *thread_fd = client_fd;

        pthread_create(&thread, NULL, handleClient, thread_fd);
    }

    close(server_fd);

    return 0;
}

void *handleClient(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    char input[1024];
    read(client_fd, input, 1024);

    Request req = {.client_fd = client_fd};

    char *section, *last;

    section = strtok_r(input, "\r\n", &last);

    // Request
    char *reqlast;
    req.method = strtok_r(section, " ", &reqlast);
    req.target = strtok_r(NULL, " ", &reqlast);
    req.version = strtok_r(NULL, " ", &reqlast);

    // Headers
    char *header_name, *header_body;
    section = strtok_r(NULL, "\r\n", &last);

    while (section != NULL) {
        // Leading spaces
        while (isspace(section[0])) section++;
        header_name = strsep(&section, ": ");
        // FIXME: why do we have to skip the space ourselves?
        header_body = ++section;
        setHeader(&req, header_name, header_body);
        section = strtok_r(NULL, "\r\n", &last);
    }

    if (strcmp(req.target, "/") == 0) {
        ok(&req);
    } else if (strncmp(ECHO_TARGET, req.target, strlen(ECHO_TARGET)) == 0) {
        echo(&req);
    } else if (strncmp(USER_TARGET, req.target, strlen(USER_TARGET)) == 0) {
        userAgent(&req);
    } else if (strncmp(FILE_TARGET, req.target, strlen(FILE_TARGET)) == 0) {
        file(&req);
    } else {
        notFound(&req);
    }

    freeRequest(&req);
    close(client_fd);
    return NULL;
}

void ok(Request *req) {
    const char OK[] = HTTP_OK "\r\n";
    write(req->client_fd, OK, strlen(OK));
}

void echo(Request *req) {
    const char ECHO_PREFIX[] = HTTP_OK
        "Content-Type: text/plain\r\n"
        "Content-Length: ";

    const char *input = req->target + strlen(ECHO_TARGET);
    int input_len = strlen(input);
    char *echo = malloc(strlen(ECHO_PREFIX) + snprintf(NULL, 0, "%d", input_len) +
                        strlen("\r\n\r\n") + strlen(input) + 1);
    sprintf(echo, "%s%d\r\n\r\n%s", ECHO_PREFIX, input_len, input);
    write(req->client_fd, echo, strlen(echo));
    free(echo);
}

void userAgent(Request *req) {
    const char USER_PREFIX[] = HTTP_OK
        "Content-Type: text/plain\r\n"
        "Content-Length: ";
    Header *h = getHeader(req, "User-Agent");
    char *body = h->body;
    int body_len = strlen(body);

    char *user_agent = malloc(strlen(USER_PREFIX) + snprintf(NULL, 0, "%d", body_len) +
                              strlen("\r\n\r\n") + strlen(body) + 1);
    sprintf(user_agent, "%s%d\r\n\r\n%s", USER_PREFIX, body_len, body);
    write(req->client_fd, user_agent, strlen(user_agent));
    free(user_agent);
}

void file(Request *req) {
    const char FILE_PREFIX[] = HTTP_OK
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: ";
    const char *input = req->target + strlen(FILE_TARGET);
    char path[1024];

    strcpy(path, dir);
    strcpy(path + strlen(dir), input);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        notFound(req);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *get_file = malloc(strlen(FILE_PREFIX) + snprintf(NULL, 0, "%ld", size) +
                            strlen("\r\n\r\n") + size + 1);
    sprintf(get_file, "%s%ld\r\n\r\n", FILE_PREFIX, size);

    fread(get_file + strlen(get_file), 1, size, fp);
    write(req->client_fd, get_file, strlen(get_file));

    free(get_file);
    fclose(fp);
}

void notFound(Request *req) {
    const char NOT_FOUND[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    write(req->client_fd, NOT_FOUND, strlen(NOT_FOUND));
}
