#include "proxy_parse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

#define DEFAULT_NHDRS 8
#define MAX_REQ_LEN 65535
#define MIN_REQ_LEN 4

static const char *root_abs_path = "/";

/* Private function declarations */
static int ParsedRequest_printRequestLine(struct ParsedRequest *pr, 
                                        char *buf, size_t buflen,
                                        size_t *tmp);
static size_t ParsedRequest_requestLineLen(struct ParsedRequest *pr);

void debug(const char *format, ...) {
    va_list args;
    if (DEBUG) {
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
}

/* ParsedHeader Public Methods */
int ParsedHeader_set(struct ParsedRequest *pr, const char *key, const char *value) {
    if (!pr || !key || !value) return -1;
    
    ParsedHeader_remove(pr, key);

    if (pr->headerslen <= pr->headersused + 1) {
        size_t new_len = pr->headerslen * 2;
        struct ParsedHeader *new_headers = realloc(pr->headers, new_len * sizeof(struct ParsedHeader));
        if (!new_headers) return -1;
        
        pr->headers = new_headers;
        pr->headerslen = new_len;
    }

    struct ParsedHeader *ph = &pr->headers[pr->headersused++];
    ph->key = strdup(key);
    ph->value = strdup(value);
    
    if (!ph->key || !ph->value) {
        free(ph->key);
        free(ph->value);
        return -1;
    }
    
    ph->keylen = strlen(key) + 1;
    ph->valuelen = strlen(value) + 1;
    return 0;
}

struct ParsedHeader *ParsedHeader_get(struct ParsedRequest *pr, const char *key) {
    if (!pr || !key) return NULL;
    
    for (size_t i = 0; i < pr->headersused; i++) {
        struct ParsedHeader *tmp = &pr->headers[i];
        if (tmp->key && strcmp(tmp->key, key) == 0) {
            return tmp;
        }
    }
    return NULL;
}

int ParsedHeader_remove(struct ParsedRequest *pr, const char *key) {
    struct ParsedHeader *ph = ParsedHeader_get(pr, key);
    if (!ph) return -1;
    
    free(ph->key);
    free(ph->value);
    ph->key = NULL;
    ph->value = NULL;
    return 0;
}

int ParsedHeader_modify(struct ParsedRequest *pr, const char *key, const char *newValue) {
    struct ParsedHeader *ph = ParsedHeader_get(pr, key);
    if (!ph) return 0;
    
    if (ph->valuelen < strlen(newValue) + 1) {
        char *new_val = realloc(ph->value, strlen(newValue) + 1);
        if (!new_val) return 0;
        ph->value = new_val;
        ph->valuelen = strlen(newValue) + 1;
    }
    strcpy(ph->value, newValue);
    return 1;
}

/* ParsedHeader Private Methods */
static void ParsedHeader_create(struct ParsedRequest *pr) {
    pr->headers = calloc(DEFAULT_NHDRS, sizeof(struct ParsedHeader));
    pr->headerslen = DEFAULT_NHDRS;
    pr->headersused = 0;
}

static size_t ParsedHeader_lineLen(struct ParsedHeader *ph) {
    return ph->key ? strlen(ph->key) + strlen(ph->value) + 4 : 0;
}

static size_t ParsedHeader_headersLen(struct ParsedRequest *pr) {
    if (!pr || !pr->buf) return 0;
    
    size_t len = 0;
    for (size_t i = 0; i < pr->headersused; i++) {
        len += ParsedHeader_lineLen(&pr->headers[i]);
    }
    return len + 2; // For \r\n
}

static int ParsedHeader_printHeaders(struct ParsedRequest *pr, char *buf, size_t len) {
    if (!pr || !buf || len < ParsedHeader_headersLen(pr)) {
        debug("buffer for printing headers too small\n");
        return -1;
    }

    char *current = buf;
    for (size_t i = 0; i < pr->headersused; i++) {
        struct ParsedHeader *ph = &pr->headers[i];
        if (ph->key) {
            size_t key_len = strlen(ph->key);
            size_t val_len = strlen(ph->value);
            
            memcpy(current, ph->key, key_len);
            current += key_len;
            memcpy(current, ": ", 2);
            current += 2;
            memcpy(current, ph->value, val_len);
            current += val_len;
            memcpy(current, "\r\n", 2);
            current += 2;
        }
    }
    memcpy(current, "\r\n", 2);
    return 0;
}

static void ParsedHeader_destroyOne(struct ParsedHeader *ph) {
    if (ph->key) {
        free(ph->key);
        free(ph->value);
        ph->key = ph->value = NULL;
        ph->keylen = ph->valuelen = 0;
    }
}

static void ParsedHeader_destroy(struct ParsedRequest *pr) {
    for (size_t i = 0; i < pr->headersused; i++) {
        ParsedHeader_destroyOne(&pr->headers[i]);
    }
    free(pr->headers);
    pr->headerslen = pr->headersused = 0;
}

static int ParsedHeader_parse(struct ParsedRequest *pr, char *line) {
    if (!pr || !line) return -1;

    char *colon = strchr(line, ':');
    if (!colon || colon == line) {
        debug("Invalid header format\n");
        return -1;
    }

    // Skip whitespace after colon
    char *value_start = colon + 1;
    while (*value_start == ' ' || *value_start == '\t') value_start++;

    char *line_end = strstr(line, "\r\n");
    if (!line_end || value_start >= line_end) {
        debug("Invalid header value\n");
        return -1;
    }

    // Extract key and value
    size_t key_len = colon - line;
    char *key = malloc(key_len + 1);
    if (!key) return -1;
    
    size_t val_len = line_end - value_start;
    char *value = malloc(val_len + 1);
    if (!value) {
        free(key);
        return -1;
    }

    strncpy(key, line, key_len);
    key[key_len] = '\0';
    strncpy(value, value_start, val_len);
    value[val_len] = '\0';

    int result = ParsedHeader_set(pr, key, value);
    free(key);
    free(value);
    return result;
}

/* ParsedRequest Public Methods */
void ParsedRequest_destroy(struct ParsedRequest *pr) {
    if (!pr) return;
    
    free(pr->buf);
    free(pr->path);
    ParsedHeader_destroy(pr);
    free(pr);
}

struct ParsedRequest *ParsedRequest_create() {
    struct ParsedRequest *pr = calloc(1, sizeof(struct ParsedRequest));
    if (pr) {
        ParsedHeader_create(pr);
    }
    return pr;
}

int ParsedRequest_unparse(struct ParsedRequest *pr, char *buf, size_t buflen) {
    if (!pr || !buf) return -1;
    
    size_t tmp;
    return ParsedRequest_printRequestLine(pr, buf, buflen, &tmp) == 0 && 
           ParsedHeader_printHeaders(pr, buf + tmp, buflen - tmp) == 0 ? 0 : -1;
}

int ParsedRequest_unparse_headers(struct ParsedRequest *pr, char *buf, size_t buflen) {
    return pr && buf ? ParsedHeader_printHeaders(pr, buf, buflen) : -1;
}

size_t ParsedRequest_totalLen(struct ParsedRequest *pr) {
    return pr ? ParsedRequest_requestLineLen(pr) + ParsedHeader_headersLen(pr) : 0;
}

int ParsedRequest_parse(struct ParsedRequest *pr, const char *buf, int buflen) {
    if (!pr || !buf || buflen < MIN_REQ_LEN || buflen > MAX_REQ_LEN) {
        debug("Invalid parameters\n");
        return -1;
    }

    char *tmp_buf = malloc(buflen + 1);
    if (!tmp_buf) return -1;
    memcpy(tmp_buf, buf, buflen);
    tmp_buf[buflen] = '\0';

    char *end_of_headers = strstr(tmp_buf, "\r\n\r\n");
    if (!end_of_headers) {
        free(tmp_buf);
        return -1;
    }

    char *end_of_first_line = strstr(tmp_buf, "\r\n");
    if (!end_of_first_line) {
        free(tmp_buf);
        return -1;
    }

    // Parse request line
    *end_of_first_line = '\0';
    char *saveptr;
    pr->method = strtok_r(tmp_buf, " ", &saveptr);
    char *uri = strtok_r(NULL, " ", &saveptr);
    pr->version = strtok_r(NULL, " ", &saveptr);

    if (!pr->method || !uri || !pr->version) {
        free(tmp_buf);
        return -1;
    }

    if (strcmp(pr->method, "GET") != 0) {
        free(tmp_buf);
        return -1;
    }

    if (strncmp(pr->version, "HTTP/", 5) != 0) {
        free(tmp_buf);
        return -1;
    }

    // Parse URI
    char *protocol = strtok_r(uri, "://", &saveptr);
    if (!protocol) {
        free(tmp_buf);
        return -1;
    }

    char *host_port = strtok_r(NULL, "/", &saveptr);
    char *path = strtok_r(NULL, " ", &saveptr);

    pr->protocol = protocol;
    pr->host = strtok_r(host_port, ":", &saveptr);
    pr->port = strtok_r(NULL, "/", &saveptr);
    
    if (!pr->host) {
        free(tmp_buf);
        return -1;
    }

    if (pr->port) {
        char *end;
        long port = strtol(pr->port, &end, 10);
        if (*end != '\0' || port < 1 || port > 65535) {
            free(tmp_buf);
            return -1;
        }
    }

    // Handle path
    if (!path) {
        pr->path = strdup(root_abs_path);
    } else {
        size_t path_len = strlen(path);
        pr->path = malloc(strlen(root_abs_path) + path_len + 1);
        if (!pr->path) {
            free(tmp_buf);
            return -1;
        }
        strcpy(pr->path, root_abs_path);
        strcat(pr->path, path);
    }

    // Parse headers
    char *current = end_of_first_line + 2;
    while (current < end_of_headers) {
        char *next = strstr(current, "\r\n");
        if (!next || next >= end_of_headers) break;
        
        if (ParsedHeader_parse(pr, current) != 0) {
            free(tmp_buf);
            return -1;
        }
        current = next + 2;
    }

    free(tmp_buf);
    return 0;
}

/* ParsedRequest Private Methods */
static size_t ParsedRequest_requestLineLen(struct ParsedRequest *pr) {
    if (!pr || !pr->buf) return 0;
    
    size_t len = strlen(pr->method) + 1 + strlen(pr->protocol) + 3 +
                strlen(pr->host) + 1 + strlen(pr->version) + 2;
    if (pr->port) len += strlen(pr->port) + 1;
    len += strlen(pr->path);
    return len;
}

static int ParsedRequest_printRequestLine(struct ParsedRequest *pr, 
                                        char *buf, size_t buflen,
                                        size_t *tmp) {
    if (!pr || !buf || buflen < ParsedRequest_requestLineLen(pr)) {
        debug("not enough memory for first line\n");
        return -1;
    }

    char *current = buf;
    size_t n;

    n = strlen(pr->method);
    memcpy(current, pr->method, n);
    current += n;
    *current++ = ' ';

    n = strlen(pr->protocol);
    memcpy(current, pr->protocol, n);
    current += n;
    memcpy(current, "://", 3);
    current += 3;

    n = strlen(pr->host);
    memcpy(current, pr->host, n);
    current += n;

    if (pr->port) {
        *current++ = ':';
        n = strlen(pr->port);
        memcpy(current, pr->port, n);
        current += n;
    }

    n = strlen(pr->path);
    memcpy(current, pr->path, n);
    current += n;

    *current++ = ' ';

    n = strlen(pr->version);
    memcpy(current, pr->version, n);
    current += n;
    memcpy(current, "\r\n", 2);
    current += 2;

    if (tmp) *tmp = current - buf;
    return 0;
}
