#include "context.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/datastructures.h"
#include "utils/shell.h"

char* context_collect(const Config* config) {
  int n = config->context_commands_count;
  if (n == 0) return strdup("");

  /* collect "$ cmd\n" headers and outputs as alternating parts */
  int nparts = n * 2;
  char** parts = malloc(sizeof(char*) * nparts);
  char** headers = malloc(sizeof(char*) * n);

  for (int i = 0; i < n; i++) {
    const char* cmd = config->context_commands[i];
    char header[CL_MAX_STR + 32];
    snprintf(header, sizeof(header), "$ %s\n", cmd);
    headers[i] = strdup(header);
    parts[i * 2] = headers[i];
    parts[i * 2 + 1] = read_popen(cmd);
  }

  char* result = join((const char**)parts, nparts, "\n");

  for (int i = 0; i < n; i++) {
    free(headers[i]);
    free(parts[i * 2 + 1]);
  }
  free(headers);
  free(parts);
  return result;
}
