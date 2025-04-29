#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    
    // Valores por defecto
    char *host = "localhost";
    char *method = "GET";
    char *path = "/";
    char *data = NULL;
    portno = 80;

    // Parsear argumentos
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 && i+1 < argc) {
            host = argv[++i];
            // Extraer puerto si existe
            char *port_str = strchr(host, ':');
            if (port_str) {
                *port_str = '\0';
                portno = atoi(port_str + 1);
            }
        }
        else if (strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            method = argv[++i];
        }
        else if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            path = argv[++i];
        }
        else if (strcmp(argv[i], "-d") == 0 && i+1 < argc) {
            data = argv[++i];
        }
    }

    // Crear socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // Obtener información del servidor
    server = gethostbyname(host);
    if (server == NULL) error("ERROR no such host");

    // Configurar estructura de dirección
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Conectar al servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");

    // Construir request HTTP
    char request[BUFFER_SIZE];
    if (data) {
        snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n"
            "%s",
            method, path, host, strlen(data), data);
    } else {
        snprintf(request, sizeof(request),
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",
            method, path, host);
    }

    // Enviar request
    if (send(sockfd, request, strlen(request), 0) < 0)
        error("ERROR sending request");

    // Recibir respuesta
    int total = 0, n;
    while ((n = recv(sockfd, buffer + total, sizeof(buffer) - total - 1, 0)) > 0) {
        total += n;
        buffer[total] = '\0';
    }
    if (n < 0) error("ERROR reading response");

    printf("%s\n", buffer);
    close(sockfd);
    return 0;
}