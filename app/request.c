#include "request.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compress.h"
#include "unistd.h"

#define MAX_BUF_SIZE 1024
#define eos(s) (s + strlen(s))

const char* STATUS_STR[] = {"200 OK", "201 Created", "404 Not Found"};
const char* CONTENT_TYPE_STR[] = {"text/plain", "application/octet-stream"};

/// Convert a string to a METHOD enum.
METHOD method_from_string(char* str) {
    if (strcmp(str, "GET") == 0) {
        return GET;
    } else if (strcmp(str, "POST") == 0) {
        return POST;
    } else {
        return UNSUPPORTED;
    }
}

/// Get a header by name from a request if one exists, otherwise, return `NULL`.
Header* req_get_header(Request* req, char* name) {
    Header* h = req->headers;
    while (h != NULL) {
        if (strcmp(h->name, name) == 0) {
            return h;
        }
        h = h->_next;
    }
    return NULL;
}

/// Add a new header to the request.
///
/// Only do this when sure that an existing header of this name is not set,
/// otherwise, could create duplicates. See the safer `setHeader`.
void req_add_header(Request* req, char* name, char* body) {
    Header* new = malloc(sizeof(Header));
    new->name = name;
    new->body = body;
    new->_next = NULL;

    if (req->headers == NULL) {
        req->headers = new;
    } else {
        Header* h = req->headers;
        while (h->_next != NULL) {
            h = h->_next;
        }
        h->_next = new;
    }
}

/// Set an existing header if one exists, otherwise add one.
void req_set_header(Request* req, char* name, char* body) {
    Header* h = req_get_header(req, name);
    if (h != NULL) {
        h->name = name;
        h->body = body;
    } else {
        req_add_header(req, name, body);
    }
}

/// Create a request from an input buffer and a client fd.
Request req_init(char* buf, int fd) {
    Request req = {._fd = fd};

    // Split request/header and body
    req.body = strstr(buf, "\r\n\r\n");
    *req.body = '\0';
    req.body += strlen("\r\n\r\n");

    // Split request line and headers
    char *section, *last;
    section = strtok_r(buf, "\r\n", &last);

    // Request
    char* reqlast;
    req.method = method_from_string(strtok_r(section, " ", &reqlast));
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
        req_set_header(&req, header_name, header_body);
        section = strtok_r(NULL, "\r\n", &last);
    }

    return req;
}

/// Free all allocated fields in a Request.
void req_free(Request* req) {
    Header* h = req->headers;
    while (h != NULL) {
        Header* temp = h;
        h = h->_next;
        free(temp);
    }
}

void req_write(Request* req, Response* res) {
    char buf[MAX_BUF_SIZE];
    int len;

    sprintf(buf, "HTTP/1.1 %s\r\n", STATUS_STR[res->status]);

    if (res->body != NULL) {
        Header* accept_encoding = req_get_header(req, "Accept-Encoding");
        len = strlen(res->body);
        if (accept_encoding != NULL) {
            char* last;
            for (char* encoding = strtok_r(accept_encoding->body, ", ", &last); encoding != NULL;
                 encoding = strtok_r(NULL, ", ", &last)) {
                if (strcmp(encoding, "gzip") == 0) {
                    char compressed[MAX_BUF_SIZE];
                    len = gzip(res->body, strlen(res->body), compressed, sizeof(compressed));
                    res->body = compressed;
                    strcat(buf, "Content-Encoding: gzip\r\n");
                    break;
                }
            }
        }
        sprintf(eos(buf), "Content-Type: %s\r\n", CONTENT_TYPE_STR[res->content_type]);
        sprintf(eos(buf), "Content-Length: %d\r\n\r\n", len);
    } else {
        res->body = "\r\n";
        len = strlen(res->body);
    }
    write(req->_fd, buf, strlen(buf));
    write(req->_fd, res->body, len);
}
