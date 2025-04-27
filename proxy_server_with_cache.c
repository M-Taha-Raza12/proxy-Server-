#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_BYTES 4096
#define MAX_CLIENTS 400
#define MAX_CACHE_SIZE (200 * (1 << 20))  // 200MB
#define MAX_ELEMENT_SIZE (10 * (1 << 20)) // 10MB

typedef struct cache_element {
    char *data;
    int len;
    char *url;
    time_t lru_time;
    struct cache_element *next;
} cache_element;

typedef struct {
    cache_element *head;
    int size;
    pthread_mutex_t lock;
} cache_t;

cache_t cache = {NULL, 0, PTHREAD_MUTEX_INITIALIZER};
int proxy_socket;
pthread_t tid[MAX_CLIENTS];
sem_t client_sem;

// Cache functions
cache_element *cache_find(const char *url) {
    pthread_mutex_lock(&cache.lock);
    cache_element *current = cache.head;
    cache_element *prev = NULL;
    cache_element *found = NULL;

    while (current) {
        if (strcmp(current->url, url) == 0) {
            found = current;
            // Move to front (LRU)
            if (prev) {
                prev->next = current->next;
                current->next = cache.head;
                cache.head = current;
            }
            current->lru_time = time(NULL);
            break;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&cache.lock);
    return found;
}

void cache_remove_oldest() {
    if (!cache.head) return;

    cache_element *prev = NULL;
    cache_element *current = cache.head;
    cache_element *oldest_prev = NULL;
    cache_element *oldest = cache.head;
    time_t oldest_time = cache.head->lru_time;

    while (current) {
        if (current->lru_time < oldest_time) {
            oldest_time = current->lru_time;
            oldest = current;
            oldest_prev = prev;
        }
        prev = current;
        current = current->next;
    }

    if (oldest_prev) {
        oldest_prev->next = oldest->next;
    } else {
        cache.head = oldest->next;
    }

    cache.size -= oldest->len + strlen(oldest->url) + sizeof(cache_element);
    free(oldest->data);
    free(oldest->url);
    free(oldest);
}

int cache_add(const char *data, int size, const char *url) {
    if (size > MAX_ELEMENT_SIZE) return 0;

    pthread_mutex_lock(&cache.lock);
    
    // Make space if needed
    while (cache.size + size + strlen(url) + sizeof(cache_element) > MAX_CACHE_SIZE) {
        cache_remove_oldest();
    }

    cache_element *element = malloc(sizeof(cache_element));
    if (!element) {
        pthread_mutex_unlock(&cache.lock);
        return 0;
    }

    element->data = malloc(size);
    element->url = strdup(url);
    if (!element->data || !element->url) {
        free(element->data);
        free(element->url);
        free(element);
        pthread_mutex_unlock(&cache.lock);
        return 0;
    }

    memcpy(element->data, data, size);
    element->len = size;
    element->lru_time = time(NULL);
    element->next = cache.head;
    cache.head = element;
    cache.size += size + strlen(url) + sizeof(cache_element);

    pthread_mutex_unlock(&cache.lock);
    return 1;
}

// Proxy functions
int connect_to_server(const char *host, int port) {
    struct hostent *server = gethostbyname(host);
    if (!server) return -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    char buffer[MAX_BYTES] = {0};
    int bytes_read;

    sem_wait(&client_sem);

    // Read client request
    bytes_read = recv(client_sock, buffer, MAX_BYTES, 0);
    if (bytes_read <= 0) {
        close(client_sock);
        sem_post(&client_sem);
        return NULL;
    }

    // Check cache
    cache_element *cached = cache_find(buffer);
    if (cached) {
        send(client_sock, cached->data, cached->len, 0);
        close(client_sock);
        sem_post(&client_sem);
        return NULL;
    }

    // Parse request (simplified - in practice use proper HTTP parsing)
    char *host = strstr(buffer, "Host: ");
    if (!host) {
        close(client_sock);
        sem_post(&client_sem);
        return NULL;
    }
    host += 6; // Skip "Host: "
    char *host_end = strstr(host, "\r\n");
    if (!host_end) {
        close(client_sock);
        sem_post(&client_sem);
        return NULL;
    }
    *host_end = '\0';

    // Connect to remote server
    int server_sock = connect_to_server(host, 80);
    if (server_sock < 0) {
        close(client_sock);
        sem_post(&client_sem);
        return NULL;
    }

    // Forward request
    send(server_sock, buffer, bytes_read, 0);

    // Read response and forward to client
    char response[MAX_BYTES];
    char *full_response = NULL;
    int full_size = 0;
    while ((bytes_read = recv(server_sock, response, MAX_BYTES, 0)) > 0) {
        send(client_sock, response, bytes_read, 0);
        
        // Cache the response
        char *temp = realloc(full_response, full_size + bytes_read);
        if (!temp) break;
        full_response = temp;
        memcpy(full_response + full_size, response, bytes_read);
        full_size += bytes_read;
    }

    // Add to cache if successful
    if (full_response && full_size > 0) {
        cache_add(full_response, full_size, buffer);
    }

    free(full_response);
    close(server_sock);
    close(client_sock);
    sem_post(&client_sem);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        return 1;
    }

    // Initialize semaphore
    sem_init(&client_sem, 0, MAX_CLIENTS);

    // Create server socket
    proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_socket < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(proxy_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(proxy_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(proxy_socket);
        return 1;
    }

    if (listen(proxy_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        close(proxy_socket);
        return 1;
    }

    printf("Proxy server listening on port %d...\n", port);

    // Main loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(proxy_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        // Create thread for client
        if (pthread_create(&tid[client_sock % MAX_CLIENTS], NULL, handle_client, &client_sock) != 0) {
            perror("pthread_create");
            close(client_sock);
        }
    }

    close(proxy_socket);
    sem_destroy(&client_sem);
    return 0;
}
