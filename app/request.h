// TODO: realloc-based vector instead of linked-list
typedef struct header {
    struct header* _next;
    char* name;
    char* body;
} Header;

typedef enum {
    GET = 0,
    POST,
    UNSUPPORTED,
} METHOD;

typedef enum {
    HTTP_OK = 0,
    HTTP_CREATED,
    HTTP_NOT_FOUND,
} STATUS;

typedef enum {
    TEXT_PLAIN = 0,
    APP_OCTET_STREAM,
} CONTENT_TYPE;

typedef struct {
    int _client_fd;
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

void req_free(Request *req);
Header* req_get_header(Request* req, char* name);
void req_add_header(Request* req, char* name, char* body);
void req_set_header(Request* req, char* name, char* body);
void req_write(Request *req, Response *res);
