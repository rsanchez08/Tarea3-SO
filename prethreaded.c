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
#include <ctype.h>
#include <inttypes.h>

#define MAX_THREADS 2
#define QUEUE_SIZE 3
#define BUFFER_SIZE 4096
#define PROTOCOL_MAX 16
#define MAX_HEADERS 20
#define MAX_CONTENT_LENGTH 1048576

typedef struct {
    int sockets[QUEUE_SIZE];
    int front, rear, count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;

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

void send_response(int client, const char *response) {
    send(client, response, strlen(response), 0);
    close(client);
}

void url_decode(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)*(src+1)) && isxdigit((unsigned char)*(src+2))) {
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
    if (!line) return -1;
    
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

const char* get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    struct {
        const char *ext;
        const char *mime;
    } mime_types[] = {
        {".html", "text/html"}, {".css", "text/css"},
        {".js", "application/javascript"}, {".json", "application/json"},
        {".jpg", "image/jpeg"}, {".png", "image/png"},
        {".gif", "image/gif"}, {".txt", "text/plain"},
        {".zip", "application/zip"}
    };

    for (size_t i = 0; i < sizeof(mime_types)/sizeof(mime_types[0]); i++) {
        if (strcasecmp(ext, mime_types[i].ext) == 0) {
            return mime_types[i].mime;
        }
    }
    return "application/octet-stream";
}

void handle_get(int client, HTTPRequest *req) {
    sleep(2);  // Simula procesamiento lento
    
    char full_path[512], decoded_path[512];
    struct stat st;
    
    strncpy(decoded_path, req->path, 511);
    url_decode(decoded_path);
    
    if (strstr(decoded_path, "..")) {
        send_response(client, "HTTP/1.1 403 Forbidden\r\n\r\n");
        return;
    }
    
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
    
    const char *mime_type = get_mime_type(full_path);
    char headers[1024];
    snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %" PRId64 "\r\n"
        "Connection: close\r\n\r\n",
        mime_type,
        (int64_t)st.st_size);
    
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

void* worker(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue.mutex);
        while (queue.count == 0)
            pthread_cond_wait(&queue.cond, &queue.mutex);
        
        int client = queue.sockets[queue.front];
        queue.front = (queue.front + 1) % QUEUE_SIZE;
        queue.count--;
        pthread_mutex_unlock(&queue.mutex);

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

        if (strcmp(req.method, "GET") == 0) {
            handle_get(client, &req);
        } else {
            send_response(client, "HTTP/1.1 501 Not Implemented\r\n\r\n");
        }
        
        close(client);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt, num_threads = MAX_THREADS;
    port = 8080;
    web_root = ".";

    while ((opt = getopt(argc, argv, "n:w:p:")) != -1) {
        switch (opt) {
            case 'n': num_threads = atoi(optarg); break;
            case 'w': web_root = optarg; break;
            case 'p': port = atoi(optarg); break;
            default: break;
        }
    }

    struct stat st = {0};
    if (stat(web_root, &st) == -1) {
        fprintf(stderr, "Error: Directorio web '%s' no existe\n", web_root);
        exit(EXIT_FAILURE);
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

    printf("Servidor iniciado en puerto %d\n", port);
    printf("Hilos trabajadores: %d\n", num_threads);
    printf("Conexiones maximas: %d\n", num_threads + QUEUE_SIZE);

    while (1) {
        int client = accept(server, NULL, NULL);
        if (client == -1) continue;

        pthread_mutex_lock(&queue.mutex);
        if (queue.count == QUEUE_SIZE) {
            printf("[🛑] Conexión rechazada (503) | En cola: %d\n", queue.count);
            const char *error_body = 
                "<html><body>"
                "<h1>503 Service Unavailable</h1>"
                "<p>El servidor no puede aceptar mas conexiones</p>"
                "</body></html>";
            
            char error_response[512];
            snprintf(error_response, sizeof(error_response),
                "HTTP/1.1 503 Service Unavailable\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n\r\n%s",
                strlen(error_body), error_body);
            
            send(client, error_response, strlen(error_response), 0);
            close(client);
        } else {
            printf("[✅] Conexión aceptada | En cola: %d\n", queue.count + 1);
            queue.sockets[queue.rear] = client;
            queue.rear = (queue.rear + 1) % QUEUE_SIZE;
            queue.count++;
            pthread_cond_signal(&queue.cond);
        }
        pthread_mutex_unlock(&queue.mutex);
    }
}