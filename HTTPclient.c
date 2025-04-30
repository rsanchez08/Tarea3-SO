#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct UploadData {
    const char *data;
    size_t length;
    size_t bytes_read;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        fprintf(stderr, "Error de memoria!\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

static size_t ReadUploadData(void *ptr, size_t size, size_t nmemb, void *userp) {
    struct UploadData *upload = (struct UploadData *)userp;
    size_t to_copy = upload->length - upload->bytes_read;
    
    if(size * nmemb < to_copy)
        to_copy = size * nmemb;
    
    if(to_copy) {
        memcpy(ptr, upload->data + upload->bytes_read, to_copy);
        upload->bytes_read += to_copy;
    }
    
    return to_copy;
}

void print_response(struct MemoryStruct *chunk, CURL *curl) {
    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    printf("\n=== Respuesta ===\n");
    printf("Status: %ld\n", http_code);
    if(chunk->size > 0) {
        printf("Contenido (%zu bytes):\n%.*s\n", 
               chunk->size, (int)chunk->size, chunk->memory);
    }
}

int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    struct UploadData upload_data = {0};
    
    char *url = NULL;
    char *method = "GET";
    char *post_data = NULL;
    char *upload_file = NULL;
    struct curl_slist *headers = NULL;

    // Parsear argumentos
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-X") == 0 && i+1 < argc) {
            method = argv[++i];
        } else if(strcmp(argv[i], "-d") == 0 && i+1 < argc) {
            post_data = argv[++i];
        } else if(strcmp(argv[i], "-T") == 0 && i+1 < argc) {
            upload_file = argv[++i];
        } else if(strcmp(argv[i], "-H") == 0 && i+1 < argc) {
            headers = curl_slist_append(headers, argv[++i]);
        } else if(strcmp(argv[i], "-u") == 0 && i+1 < argc) {
            url = argv[++i];
        }
    }

    if(!url) {
        fprintf(stderr, "Uso: %s -u URL [OPCIONES]\n"
                "Opciones:\n"
                "  -X METODO   (GET|POST|PUT|DELETE|HEAD)\n"
                "  -d DATA     Datos para POST\n"
                "  -T ARCHIVO  Archivo para PUT\n"
                "  -H HEADER   Cabecera personalizada\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // Configuración común
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "CustomHTTPClient/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // Configurar método
        if(strcasecmp(method, "POST") == 0) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if(post_data) {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post_data));
            }
        } else if(strcasecmp(method, "PUT") == 0) {
            if(upload_file) {
                FILE *f = fopen(upload_file, "rb");
                if(f) {
                    fseek(f, 0, SEEK_END);
                    long fsize = ftell(f);
                    fseek(f, 0, SEEK_SET);
                    
                    char *data = malloc(fsize);
                    fread(data, 1, fsize, f);
                    fclose(f);
                    
                    upload_data.data = data;
                    upload_data.length = fsize;
                    
                    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
                    curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadUploadData);
                    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_data);
                    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
                }
            }
        } else if(strcasecmp(method, "DELETE") == 0) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        } else if(strcasecmp(method, "HEAD") == 0) {
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        }
        
        // Cabeceras personalizadas
        if(headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        // Ejecutar solicitud
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "Error CURL: %s\n", curl_easy_strerror(res));
        } else {
            print_response(&chunk, curl);
        }
        
        // Limpieza
        curl_easy_cleanup(curl);
        if(upload_data.data) free((void *)upload_data.data);
        if(chunk.memory) free(chunk.memory);
        if(headers) curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return 0;
}