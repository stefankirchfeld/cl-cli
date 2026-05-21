#include "shell.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datastructures.h"

char* ls_dir(const char* path) {
  DIR* d = opendir(path);
  if (!d) return strdup("");
  size_t cap = 4096, len = 0;
  char* buf = malloc(cap);
  buf[0] = '\0';
  struct dirent* e;
  while ((e = readdir(d))) {
    if (str_equals(e->d_name, ".") || str_equals(e->d_name, "..")) continue;
    size_t nl = strlen(e->d_name);
    if (len + nl + 2 > cap) {
      cap *= 2;
      buf = realloc(buf, cap);
    }
    memcpy(buf + len, e->d_name, nl);
    len += nl;
    buf[len++] = '\n';
    buf[len] = '\0';
  }
  closedir(d);
  return buf;
}

char* read_popen(const char* cmd) {
  FILE* fp = popen(cmd, "r");
  if (!fp) return strdup("");
  size_t cap = 4096, len = 0;
  char* buf = malloc(cap);
  buf[0] = '\0';
  char tmp[4096];
  size_t n;
  while ((n = fread(tmp, 1, sizeof(tmp), fp)) > 0) {
    if (len + n + 1 > cap) {
      cap = (len + n + 1) * 2;
      buf = realloc(buf, cap);
    }
    memcpy(buf + len, tmp, n);
    len += n;
    buf[len] = '\0';
  }
  pclose(fp);
  return buf;
}

