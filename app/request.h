// TODO: realloc-based vector instead of linked-list
typedef struct header {
    struct header* _next;
    char* name;
    char* body;
} Header;

typedef enum {
    GET,
    POST,
    UNSUPPORTED,
} METHOD;

typedef enum {
    HTTP_OK,
    HTTP_CREATED,
    HTTP_NOT_FOUND,
} STATUS;

typedef enum {
    TEXT_PLAIN,
    APP_OCTET_STREAM,
} CONTENT_TYPE;

typedef struct {
    int _fd;
    METHOD method;
    char* target;
    char* version;
    Header* headers;
    char* body;
} Request;

// TODO: the content_* be Headers?

typedef struct {
    STATUS status;
    CONTENT_TYPE content_type;
    char* body;
} Response;

METHOD method_from_string(char* str);

Request req_init(char* buf, int fd);
void req_free(Request* req);
Header* req_get_header(Request* req, char* name);
void req_write(Request* req, Response* res);
