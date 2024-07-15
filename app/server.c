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

// Forward Declarations
void *handle_client(void *arg);
void ok(Request *req);
void echo(Request *req);
void user_agent(Request *req);
void file_get(Request *req, char *path);
void file_post(Request *req, char *path);
void not_found(Request *req);

// Constants
#define MAX_PATH_SIZE 1024
#define MAX_INPUT_SIZE 1024

const char ECHO_TARGET[] = "/echo/";
const char USER_TARGET[] = "/user-agent";
const char FILE_TARGET[] = "/files/";

const char DIR_ARG[] = "--directory";

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

        pthread_create(&thread, NULL, handle_client, thread_fd);
    }

    close(server_fd);

    return 0;
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    char input[MAX_INPUT_SIZE];
    read(client_fd, input, MAX_INPUT_SIZE);

    Request req = req_init(input, client_fd);

    if (strcmp(req.target, "/") == 0) {
        ok(&req);
    } else if (strncmp(ECHO_TARGET, req.target, strlen(ECHO_TARGET)) == 0) {
        echo(&req);
    } else if (strncmp(USER_TARGET, req.target, strlen(USER_TARGET)) == 0) {
        user_agent(&req);
    } else if (strncmp(FILE_TARGET, req.target, strlen(FILE_TARGET)) == 0) {
        const char *input = req.target + strlen(FILE_TARGET);
        char path[MAX_PATH_SIZE];
        strcpy(path, dir);
        strcpy(path + strlen(dir), input);
        if (req.method == GET) {
            file_get(&req, path);
        } else if (req.method == POST) {
            file_post(&req, path);
        } else {
            not_found(&req);
        }

    } else {
        not_found(&req);
    }

    req_free(&req);
    close(client_fd);
    return NULL;
}

void ok(Request *req) {
    Response res = {};
    req_write(req, &res);
}

void echo(Request *req) {
    char *input = req->target + strlen(ECHO_TARGET);
    Response res = {
        .body = input,
    };
    req_write(req, &res);
}

void user_agent(Request *req) {
    Header *h = req_get_header(req, "User-Agent");
    if (h == NULL) {
        not_found(req);
        return;
    }
    Response res = {
        .body = h->body,
    };
    req_write(req, &res);
}

void file_get(Request *req, char *path) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        not_found(req);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *body = malloc(size);
    fread(body, 1, size, fp);
    fclose(fp);

    Response res = {.body = body, .content_type = APP_OCTET_STREAM};
    req_write(req, &res);
    free(body);
}

void file_post(Request *req, char *path) {
    FILE *fp = fopen(path, "w");
    fputs(req->body, fp);
    fclose(fp);
    Response res = {
        .status = HTTP_CREATED,
    };
    req_write(req, &res);
}

void not_found(Request *req) {
    Response res = {
        .status = HTTP_NOT_FOUND,
    };
    req_write(req, &res);
}
