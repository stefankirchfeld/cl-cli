#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void history_path(char *out, size_t sz) {
    const char *home = getenv("HOME");
    snprintf(out, sz, "%s/.config/cl/history", home ? home : ".");
}

History *history_load(void) {
    History *h = calloc(1, sizeof(*h));
    h->entries = calloc(CL_HISTORY_MAX, sizeof(char *));
    h->pos = -1;

    char path[512];
    history_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return h;

    char line[512];
    while (fgets(line, sizeof(line), f) && h->count < CL_HISTORY_MAX) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0])
            h->entries[h->count++] = strdup(line);
    }
    fclose(f);
    return h;
}

void history_append(History *h, const char *entry) {
    if (!entry || !entry[0]) return;

    /* avoid consecutive duplicates */
    if (h->count > 0 && strcmp(h->entries[h->count - 1], entry) == 0) return;

    if (h->count < CL_HISTORY_MAX) {
        h->entries[h->count++] = strdup(entry);
    } else {
        /* shift out oldest */
        free(h->entries[0]);
        memmove(h->entries, h->entries + 1,
                (CL_HISTORY_MAX - 1) * sizeof(char *));
        h->entries[CL_HISTORY_MAX - 1] = strdup(entry);
    }

    char path[512];
    history_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < h->count; i++)
        fprintf(f, "%s\n", h->entries[i]);
    fclose(f);
}

void history_free(History *h) {
    if (!h) return;
    for (int i = 0; i < h->count; i++) free(h->entries[i]);
    free(h->entries);
    free(h);
}
