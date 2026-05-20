#pragma once

#define CL_HISTORY_MAX 100

typedef struct {
  char **entries; /* oldest first */
  int count;
  int pos; /* current browse position, -1 = not browsing */
} History;

/* Load history from ~/.config/cl/history. Returns heap-allocated History. */
History *history_load(void);

/* Append an entry and save. Trims to CL_HISTORY_MAX. */
void history_append(History *h, const char *entry);

void history_free(History *h);
