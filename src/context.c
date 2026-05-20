#include "context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *context_collect(const Config *config) {
    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) return strdup("");
    buf[0] = '\0';

    for (int i = 0; i < config->context_commands_count; i++) {
        const char *cmd = config->context_commands[i];

        FILE *fp = popen(cmd, "r");
        if (!fp) continue;

        /* append header */
        char header[CL_MAX_STR + 32];
        int hlen = snprintf(header, sizeof(header), "$ %s\n", cmd);
        if (len + (size_t)hlen + 1 > cap) {
            cap = (len + (size_t)hlen + 1) * 2;
            buf = realloc(buf, cap);
            if (!buf) { pclose(fp); return strdup(""); }
        }
        memcpy(buf + len, header, (size_t)hlen);
        len += (size_t)hlen;

        /* append output */
        char tmp[4096];
        size_t n;
        while ((n = fread(tmp, 1, sizeof(tmp), fp)) > 0) {
            if (len + n + 1 > cap) {
                cap = (len + n + 1) * 2;
                buf = realloc(buf, cap);
                if (!buf) { pclose(fp); return strdup(""); }
            }
            memcpy(buf + len, tmp, n);
            len += n;
        }
        pclose(fp);

        /* separator between commands */
        if (len + 2 > cap) {
            cap = (len + 2) * 2;
            buf = realloc(buf, cap);
            if (!buf) return strdup("");
        }
        buf[len++] = '\n';
        buf[len] = '\0';
    }

    return buf;
}
