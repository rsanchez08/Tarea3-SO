#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

void print_response_metadata(CURL *curl) {
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("HTTP Status: %ld\n", http_code);
}

int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {0};
    char *url = NULL;
    char *output = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-u") == 0 && i+1 < argc) {
            url = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            output = argv[++i];
        }
    }

    if (!url) {
        fprintf(stderr, "Uso: %s -u <URL> [-o archivo]\n", argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "HTTPClient/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);

        res = curl_easy_perform(curl);
        
        if (res == CURLE_OK) {
            print_response_metadata(curl);
            if (output) {
                FILE *fp = fopen(output, "wb");
                fwrite(chunk.memory, 1, chunk.size, fp);
                fclose(fp);
            } else {
                printf("%s\n", chunk.memory);
            }
        } else {
            fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
        }
        
        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    curl_global_cleanup();
    return 0;
}