#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/datastructures.h"

Config *config_load(void) {
  Config *c = calloc(1, sizeof(*c));
  if (!c)
    return NULL;

  /* defaults */
  snprintf(c->model, sizeof(c->model), "claude-haiku-4-5-20251001");

  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "cl: HOME not set\n");
    return c;
  }

  char path[1024];
  snprintf(path, sizeof(path), "%s/.config/cl/config.ini", home);

  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "cl: config not found at %s\n", path);
    return c;
  }

  char line[1024];
  char section[64] = "";

  while (fgets(line, sizeof(line), f)) {
    /* strip trailing newline */
    line[strcspn(line, "\r\n")] = '\0';

    /* skip blanks and comments */
    char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '\0' || *p == ';' || *p == '#')
      continue;

    /* section header */
    if (*p == '[') {
      char *end = strchr(p, ']');
      if (end) {
        size_t len = (size_t)(end - p - 1);
        if (len >= sizeof(section))
          len = sizeof(section) - 1;
        memcpy(section, p + 1, len);
        section[len] = '\0';
      }
      continue;
    }

    /* key = value  or  key[] = value */
    char *eq = strchr(p, '=');
    if (!eq)
      continue;
    *eq = '\0';
    char *key = p;
    char *val = eq + 1;

    /* trim key */
    char *ke = key + strlen(key) - 1;
    while (ke > key && (*ke == ' ' || *ke == '\t'))
      *ke-- = '\0';

    /* trim val */
    while (*val == ' ' || *val == '\t')
      val++;
    char *ve = val + strlen(val) - 1;
    while (ve > val && (*ve == ' ' || *ve == '\t'))
      *ve-- = '\0';

    /* detect array key (ends with []) */
    int is_array = 0;
    size_t klen = strlen(key);
    if (klen >= 2 && key[klen - 2] == '[' && key[klen - 1] == ']') {
      key[klen - 2] = '\0';
      is_array = 1;
    }

    if (str_equals(section, "claude")) {
      if (str_equals(key, "api_key"))
        snprintf(c->api_key, sizeof(c->api_key), "%s", val);
      else if (str_equals(key, "model"))
        snprintf(c->model, sizeof(c->model), "%s", val);
    } else if (str_equals(section, "context")) {
      if (is_array && str_equals(key, "commands")) {
        if (c->context_commands_count < CL_MAX_CONTEXT_CMDS) {
          snprintf(c->context_commands[c->context_commands_count], CL_MAX_STR,
                   "%s", val);
          c->context_commands_count++;
        }
      }
    }
  }

  fclose(f);
  return c;
}
