#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Vector *new_vector() {
  Vector *vec = calloc(1, sizeof(Vector));
  vec->capacity = 16;
  vec->data = calloc(vec->capacity, sizeof(void *));
  vec->len = 0;
  return vec;
}

void delete_vector(Vector *vec) {
  for (int i = 0; i < vec->len; i++)
    free(vec->data[i]);
  free(vec->data);
  free(vec);
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

void *vec_pop(Vector *vec) {
  return vec->data[--vec->len];
}

void *vec_last(Vector *vec) {
  return vec->data[vec->len - 1];
}

Map *new_map() {
  Map *map = malloc(sizeof(Map));
  map->keys = new_vector();
  map->vals = new_vector();
  return map;
}

void delete_map(Map *map) {
  delete_vector(map->vals);
  delete_vector(map->keys);
  free(map);
}

void map_put(Map *map, char *key, void *val) {
  vec_push(map->keys, key);
  vec_push(map->vals, val);
}

void *map_get(Map *map, char *key) {
  for (int i = map->keys->len - 1; i >= 0; i--) {
    if (strcmp(map->keys->data[i], key) == 0) {
      return map->vals->data[i];
    }
  }
  return NULL;
}

int *intdup(int n) {
  int *num = malloc(sizeof(int));
  *num = n;
  return num;
}

noreturn void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void expect(int line, int expected, int actual) {
  if (expected == actual)
    return;
  error("%d: %d expected, but got %d", line, expected, actual);
}

void expect_str(int line, char *expected, char *actual) {
  if (expected == NULL)
    error("%d: non-NULL is expected, but \"expected\" is NULL", line);
  if (actual == NULL)
    error("%d: non-NULL is expected, but \"actual\" is NULL", line);
  if (strcmp(expected, actual) != 0)
    error("%d: \"%s\" expected, but got \"%s\"", line, expected, actual);
  return;
}

void expect_ptr(int line, void *expected, void *actual) {
  if (expected == actual)
    return;
  error("%d: %d expected, but got %d", line, expected, actual);
}

void expect_bool(int line, bool expected, bool actual) {
  if (expected == actual)
    return;
  if (actual == true)
    error("%d: 'false' expected, but got 'true'", line);
  else
    error("%d: 'true' expected, but got 'false'", line);
}
