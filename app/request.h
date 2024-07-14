// TODO: realloc-based vector instead of linked-list
typedef struct header {
    char* name;
    char* body;
    struct header* _next;
} Header;

typedef struct {
    int client_fd;
    char* method;
    char* target;
    char* version;
    Header* headers;
} Request;

void freeRequest(Request *req);
Header* getHeader(Request* req, char* name);
void addHeader(Request* req, char* name, char* body);
void setHeader(Request* req, char* name, char* body);
