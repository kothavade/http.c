#include "request.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "unistd.h"

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

/// Free all allocated fields in a Request.
void req_free(Request* req) {
    Header* h = req->headers;
    while (h != NULL) {
        Header* temp = h;
        h = h->_next;
        free(temp);
    }
}

const char* STATUS_STR[] = {"200 OK", "201 Created", "404 Not Found"};

const char* CONTENT_TYPE_STR[] = {"text/plain", "application/octet-stream"};

#define eos(s) (s + strlen(s))

void req_write(Request* req, Response* res) {
    char buf[1024];
    int len;

    sprintf(buf, "HTTP/1.1 %s\r\n", STATUS_STR[res->status]);

    if (res->body != NULL) {
        Header* accept_encoding = req_get_header(req, "Accept-Encoding");
        if (accept_encoding != NULL && (strcmp(accept_encoding->body, "gzip") == 0)) {
            strcat(buf, "Content-Encoding: gzip\r\n");
            len = strlen(res->body);
        } else {
            len = strlen(res->body);
        }
        sprintf(eos(buf), "Content-Type: %s\r\n", CONTENT_TYPE_STR[res->content_type]);
        sprintf(eos(buf), "Content-Length: %d\r\n\r\n", len);
        strcat(buf, res->body);
    } else {
        strcat(buf, "\r\n");
    }

    write(req->_client_fd, buf, strlen(buf));
}
