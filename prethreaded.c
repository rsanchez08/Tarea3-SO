#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define MAX_THREADS 100
#define QUEUE_SIZE 100
#define BUFFER_SIZE 4096
#define PROTOCOL_MAX 16
#define MAX_HEADERS 20
#define MAX_CONTENT_LENGTH 1048576 // 1MB

typedef struct {
    int sockets[QUEUE_SIZE];
    int front, rear, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;

typedef struct {
    char protocol[PROTOCOL_MAX];
    int port;
    char* banner;
} ProtocolConfig;

typedef struct {
    char method[16];
    char path[256];
    char version[16];
    char headers[MAX_HEADERS][2][256];
    int header_count;
    long content_length;
} HTTPRequest;

Queue queue;
char *web_root;
int port;
char protocol[PROTOCOL_MAX] = "http";
ProtocolConfig protocols[] = {
    {"ftp", 21, "220 FTP Server Ready\r\n"},
    {"ssh", 22, "SSH-2.0-OpenSSH_8.2p1\r\n"},
    {"smtp", 25, "220 SMTP Service Ready\r\n"},
    {"dns", 53, ""},
    {"telnet", 23, "TELNET Server Ready\r\n"},
    {"snmp", 161, ""},
    {"http", 80, ""},
    {"https", 443, ""}
};

void send_response(int client, const char *response) {
    send(client, response, strlen(response), 0);
    close(client);
}

void url_decode(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '%' && isxdigit(*(src+1)) && isxdigit(*(src+2))) {
            *dst = (char) strtol(src+1, NULL, 16);
            src += 3;
        } else {
            *dst = *src++;
        }
        dst++;
    }
    *dst = '\0';
}

int parse_request(const char *buffer, HTTPRequest *req) {
    char *line = strtok((char*)buffer, "\r\n");
    sscanf(line, "%15s %255s %15s", req->method, req->path, req->version);
    
    req->header_count = 0;
    req->content_length = 0;
    
    while ((line = strtok(NULL, "\r\n")) && strlen(line) > 0) {
        if (req->header_count >= MAX_HEADERS) break;
        
        char name[256], value[256];
        if (sscanf(line, "%255[^:]: %255[^\r\n]", name, value) == 2) {
            strncpy(req->headers[req->header_count][0], name, 255);
            strncpy(req->headers[req->header_count][1], value, 255);
            
            if (strcasecmp(name, "Content-Length") == 0) {
                req->content_length = atol(value);
            }
            
            req->header_count++;
        }
    }
    
    return 0;
}

void handle_protocol(int client) {
    for (int i = 0; i < sizeof(protocols)/sizeof(ProtocolConfig); i++) {
        if (strcmp(protocol, protocols[i].protocol) == 0 && strlen(protocols[i].banner) > 0) {
            send(client, protocols[i].banner, strlen(protocols[i].banner), 0);
            break;
        }
    }
    close(client);
}

void handle_get(int client, HTTPRequest *req) {
    char full_path[512], decoded_path[512];
    struct stat st;
    
    strncpy(decoded_path, req->path, 511);
    url_decode(decoded_path);
    
    snprintf(full_path, sizeof(full_path), "%s%s", web_root, decoded_path);
    
    if (stat(full_path, &st) == -1) {
        send_response(client, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }
    
    if (S_ISDIR(st.st_mode)) {
        strncat(full_path, "/index.html", sizeof(full_path)-strlen(full_path)-1);
        if (stat(full_path, &st) == -1) {
            send_response(client, "HTTP/1.1 403 Forbidden\r\n\r\n");
            return;
        }
    }
    
    FILE *file = fopen(full_path, "rb");
    if (!file) {
        send_response(client, "HTTP/1.1 403 Forbidden\r\n\r\n");
        return;
    }
    
    char headers[1024];
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        st.st_size);
    
    send(client, headers, strlen(headers), 0);
    
    if (strcmp(req->method, "GET") == 0) {
        char buffer[BUFFER_SIZE];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            send(client, buffer, bytes, 0);
        }
    }
    
    fclose(file);
}

void handle_post_put(int client, HTTPRequest *req, const char *buffer) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", web_root, req->path);
    
    FILE *file = fopen(full_path, strcmp(req->method, "PUT") == 0 ? "wb" : "ab");
    if (!file) {
        send_response(client, "HTTP/1.1 403 Forbidden\r\n\r\n");
        return;
    }
    
    const char *content = strstr(buffer, "\r\n\r\n") + 4;
    fwrite(content, 1, req->content_length, file);
    fclose(file);
    
    send_response(client, "HTTP/1.1 201 Created\r\n\r\n");
}

void handle_delete(int client, HTTPRequest *req) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", web_root, req->path);
    
    if (unlink(full_path) == 0) {
        send_response(client, "HTTP/1.1 200 OK\r\n\r\n");
    } else {
        send_response(client, "HTTP/1.1 403 Forbidden\r\n\r\n");
    }
}

void* worker(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue.mutex);
        while (queue.count == 0)
            pthread_cond_wait(&queue.cond, &queue.mutex);
        
        int client = queue.sockets[queue.front];
        queue.front = (queue.front + 1) % QUEUE_SIZE;
        queue.count--;
        pthread_mutex_unlock(&queue.mutex);

        if (strcmp(protocol, "http") != 0) {
            handle_protocol(client);
            continue;
        }

        char buffer[BUFFER_SIZE];
        int bytes = recv(client, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) {
            close(client);
            continue;
        }
        buffer[bytes] = '\0';

        HTTPRequest req;
        if (parse_request(buffer, &req)) {
            send_response(client, "HTTP/1.1 400 Bad Request\r\n\r\n");
            continue;
        }

        if (strcmp(req.version, "HTTP/1.1") != 0) {
            send_response(client, "HTTP/1.1 505 HTTP Version Not Supported\r\n\r\n");
            continue;
        }

        if (strcmp(req.method, "GET") == 0 || strcmp(req.method, "HEAD") == 0) {
            handle_get(client, &req);
        } else if (strcmp(req.method, "POST") == 0 || strcmp(req.method, "PUT") == 0) {
            handle_post_put(client, &req, buffer);
        } else if (strcmp(req.method, "DELETE") == 0) {
            handle_delete(client, &req);
        } else {
            send_response(client, "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET, HEAD, POST, PUT, DELETE\r\n\r\n");
        }
        
        close(client);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt, num_threads = 4;
    port = 8080;
    web_root = ".";

    while ((opt = getopt(argc, argv, "n:w:p:r:")) != -1) {
        switch (opt) {
            case 'n': num_threads = atoi(optarg); break;
            case 'w': web_root = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 'r': 
                strncpy(protocol, optarg, PROTOCOL_MAX-1);
                protocol[PROTOCOL_MAX-1] = '\0';
                break;
        }
    }

    for (int i = 0; i < sizeof(protocols)/sizeof(ProtocolConfig); i++) {
        if (protocols[i].port == port) {
            strcpy(protocol, protocols[i].protocol);
            break;
        }
    }

    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, SOMAXCONN);

    queue.front = queue.rear = queue.count = 0;
    pthread_mutex_init(&queue.mutex, NULL);
    pthread_cond_init(&queue.cond, NULL);

    for (int i = 0; i < num_threads; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, worker, NULL);
    }

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client == -1) continue;

        pthread_mutex_lock(&queue.mutex);
        if (queue.count == QUEUE_SIZE) {
            send(client, "HTTP/1.1 503 Service Unavailable\r\n\r\n", 26, 0);
            close(client);
        } else {
            queue.sockets[queue.rear] = client;
            queue.rear = (queue.rear + 1) % QUEUE_SIZE;
            queue.count++;
            pthread_cond_signal(&queue.cond);
        }
        pthread_mutex_unlock(&queue.mutex);
    }
}