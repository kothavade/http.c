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
    char* body;
} Request;

void free_request(Request *req);
Header* get_header(Request* req, char* name);
void add_header(Request* req, char* name, char* body);
void set_header(Request* req, char* name, char* body);
