#include "request.h"

#include <stdlib.h>
#include <string.h>

/// Get a header by name from a request if one exists, otherwise, return `NULL`.
Header* get_header(Request* req, char* name) {
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
void add_header(Request* req, char* name, char* body) {
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
void set_header(Request* req, char* name, char* body) {
    Header* h = get_header(req, name);
    if (h != NULL) {
        h->name = name;
        h->body = body;
    } else {
        add_header(req, name, body);
    }
}

/// Free all allocated fields in a Request.
void free_request(Request* req) {
    Header* h = req->headers;
    while (h != NULL) {
        Header* temp = h;
        h = h->_next;
        free(temp);
    }
}
