# http.c

A multi-threaded HTTP server written from scratch in C.

Based on the CodeCrafters ["Build Your Own HTTP server" Challenge](https://app.codecrafters.io/courses/http-server/overview).

## Features

- Multi-threaded using `pthread` -- supports concurrent connections
- `gzip` compression with `zlib` if the `Accept-Encoding` header is set to include `gzip`
- GET `/echo/{str}` endpoint returns back input
- GET `/user-agent` endpoint returns the user-agent of the client
- GET `/file/{filename}` endpoint returns the contents of a file
- POST `/file/{filename}` endpoint writes the body of the request to a file
