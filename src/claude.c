#include "claude.h"
#include "config.h"

#include <llense/libcurl/curl/curl.h>
#include <llense/libjansson/jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANTHROPIC_URL "https://api.anthropic.com/v1/messages"
#define ANTHROPIC_VERSION "2023-06-01"
#define MAX_TOKENS 1024
#define INITIAL_BUF 65536

typedef struct { char *data; size_t len, cap; } Buf;

static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *ud) {
    Buf *b = (Buf *)ud;
    size_t n = size * nmemb;
    if (b->len + n + 1 > b->cap) {
        b->cap = (b->len + n + 1) * 2;
        b->data = realloc(b->data, b->cap);
        if (!b->data) return 0;
    }
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

char *claude_complete(const char *api_key, const char *model,
                      const char *system_prompt, const char *user_prompt) {
    /* build request JSON */
    json_t *root = json_object();
    json_object_set_new(root, "model", json_string(model));
    json_object_set_new(root, "max_tokens", json_integer(MAX_TOKENS));

    if (system_prompt && system_prompt[0])
        json_object_set_new(root, "system", json_string(system_prompt));

    json_t *msgs = json_array();
    json_t *msg = json_object();
    json_object_set_new(msg, "role", json_string("user"));
    json_object_set_new(msg, "content", json_string(user_prompt));
    json_array_append_new(msgs, msg);
    json_object_set_new(root, "messages", msgs);

    char *body = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    if (!body) return NULL;

    /* set up headers */
    char auth_header[CL_MAX_STR + 32];
    snprintf(auth_header, sizeof(auth_header), "x-api-key: %s", api_key);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "anthropic-version: " ANTHROPIC_VERSION);

    /* perform request */
    Buf buf = { .data = malloc(INITIAL_BUF), .len = 0, .cap = INITIAL_BUF };

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, ANTHROPIC_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(body);

    if (res != CURLE_OK) {
        fprintf(stderr, "cl: curl error: %s\n", curl_easy_strerror(res));
        free(buf.data);
        return NULL;
    }

    /* parse response */
    json_error_t err;
    json_t *resp = json_loads(buf.data, 0, &err);
    free(buf.data);

    if (!resp) {
        fprintf(stderr, "cl: JSON parse error: %s\n", err.text);
        return NULL;
    }

    /* response shape: {"content": [{"type": "text", "text": "..."}], ...} */
    json_t *content = json_object_get(resp, "content");
    if (!content || !json_is_array(content) || json_array_size(content) == 0) {
        /* check for API error */
        json_t *error = json_object_get(resp, "error");
        if (error) {
            json_t *emsg = json_object_get(error, "message");
            fprintf(stderr, "cl: API error: %s\n",
                    emsg ? json_string_value(emsg) : "unknown");
        }
        json_decref(resp);
        return NULL;
    }

    json_t *first = json_array_get(content, 0);
    json_t *text = json_object_get(first, "text");
    char *result = text ? strdup(json_string_value(text)) : NULL;
    json_decref(resp);
    return result;
}
