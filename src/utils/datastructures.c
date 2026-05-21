#include "datastructures.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define pr printf

void vec_init(Vec* v) {
  v->data = NULL;
  v->length = v->capacity = 0;
}

void immutable_vec_init(ImmutableVec* v) {
  v->data = NULL;
  v->length = 0;
}

int _vec_expand(Vec* v) {
  if (!v) {
    return 0;
  }

  if (v->capacity > v->length) {
    return 1;
  }
  int new_size = v->capacity > 0 ? v->capacity * 2 : 4;
  VecEntry* new_data = realloc(v->data, sizeof(VecEntry) * new_size);
  if (!new_data) {
    return 0;
  }
  v->data = new_data;
  v->capacity = new_size;
  return 1;
}

void vec_push(Vec* v, VecEntry value) {
  if (!_vec_expand(v)) {
    printf("Error: Vector cannot be expanded\n");
    exit(1);
  }
  v->data[v->length] = value;
  v->length++;
}

void vec_free(Vec* v) {
  if (!v) return;

  for (int i = 0; i < v->length; i++) {
    free(v->data[i].data);
  }
  free(v->data);
  v->data = NULL;
  v->length = 0;
  v->capacity = 0;
}

void immutable_vec_free(ImmutableVec* v) {
  if (!v) return;
  // entries are only borrowed, hence not freed here

  free(v->data);
  v->data = NULL;
  v->length = 0;
}

VecEntry vec_get(const Vec* v, int i) {
  if (!v || !v->data || i < 0 || i >= v->length) {
    printf("Error: Index out of bounds\n");
    exit(1);
  }

  return v->data[i];
}

VecEntry vec_pop(Vec* v) {
  if (!v || !v->data || v->length == 0) {
    printf("Error: Vector is empty\n");
    exit(1);
  }
  VecEntry ret = v->data[v->length - 1];
  v->length--;
  return ret;
}

void vec_filter(const Vec* v, ImmutableVec* result,
                int (*predicate)(VecEntry)) {
  Vec filter;
  vec_init(&filter);
  if (!v) return;

  for (int i = 0; i < v->length; i++) {
    if (predicate(v->data[i]) == 1) {
      vec_push(&filter, v->data[i]);
    }
  }

  result->data = filter.data;
  result->length = filter.length;
  return;
}

char* str_join(const char** parts, int n, const char* sep) {
  size_t sep_len = strlen(sep);
  size_t total = 1;
  for (int i = 0; i < n; i++) total += strlen(parts[i]);
  total += (n - 1) * sep_len;
  char* out = malloc(total);
  out[0] = '\0';
  for (int i = 0; i < n; i++) {
    if (i != 0) strcat(out, sep);
    strcat(out, parts[i]);
  }
  return out;
}

int str_equals(const char* one, const char* two) {
  return strcmp(one, two) == 0;
}
