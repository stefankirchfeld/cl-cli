#pragma once

typedef struct {
  void* data;
  int length;
} VecEntry;

typedef struct {
  VecEntry* data;
  int length;
  int capacity;
} Vec;

typedef struct {
  VecEntry* data;
  int length;
} ImmutableVec;

void vec_init(Vec* v);
void vec_push(Vec* v, VecEntry value);
void vec_free(Vec* v);
VecEntry vec_get(const Vec* v, int i);
VecEntry vec_pop(Vec* v);
void vec_filter(const Vec* v, ImmutableVec* result, int (*predicate)(VecEntry));

void immutable_vec_init(ImmutableVec* v);
void immutable_vec_free(ImmutableVec* v);

char* join(const char** parts, int n, const char* sep);
int str_equals(const char* one, const char* two);
