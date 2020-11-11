#include "util.h"

#include <stdarg.h> // va_start(3)
#include <stdio.h>  // fprintf(3)
#include <stdlib.h> // free(3)
#include <string.h> // strcmp(3)

char *ErrorMsg;

Args *new_Args(int argc, char **argv) {
  Args *args = malloc(sizeof(Args));
  args->argc = argc;
  args->argv = argv;
  return args;
}

void delete_Args(Args *args) {
  free(args);
}

bool Args_hasNext(Args *args) {
  return args->argc > 0;
}

char *Args_next(Args *args) {
  char *ret = *(args->argv);
  args->argc--;
  args->argv++;

  return ret;
}

Vector *new_Vector() {
  Vector *vec = calloc(1, sizeof(Vector));
  vec->capacity = 16;
  vec->data = calloc(vec->capacity, sizeof(void *));
  vec->len = 0;
  return vec;
}

void delete_Vector(Vector *vec) {
  for (int i = 0; i < vec->len; i++)
    free(vec->data[i]);
  free(vec->data);
  free(vec);
}

void Vector_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

void *Vector_pop(Vector *vec) {
  return vec->data[--vec->len];
}

void *Vector_last(Vector *vec) {
  return vec->data[vec->len - 1];
}

Map *new_Map() {
  Map *map = malloc(sizeof(Map));
  map->keys = new_Vector();
  map->vals = new_Vector();
  return map;
}

void delete_Map(Map *map) {
  delete_Vector(map->vals);
  delete_Vector(map->keys);
  free(map);
}

void Map_put(Map *map, char *key, void *val) {
  Vector_push(map->keys, key);
  Vector_push(map->vals, val);
}

void *Map_get(Map *map, char *key) {
  for (int i = map->keys->len - 1; i >= 0; i--) {
    if (strcmp(map->keys->data[i], key) == 0) {
      return map->vals->data[i];
    }
  }
  return NULL;
}

//
// StringBuffer
//

StringBuffer *new_StringBuffer() {
  StringBuffer *sb = calloc(1, sizeof(StringBuffer));

  sb->len = 0;
  sb->_body = new_Vector();
  sb->_buf_siz = 255 + 1;
  sb->_buf = calloc(sb->_buf_siz, sizeof(char));

  return sb;
}

void delete_StringBuffer(StringBuffer *sb) {
  delete_Vector(sb->_body);
  free(sb->_buf);
  free(sb);
}

void StringBuffer_append(StringBuffer *sb, char *string) {
  if (sb->_buf_len != 0) {
    sb->_buf[sb->_buf_len] = '\0';
    Vector_push(sb->_body, strdup(sb->_buf));
    sb->_buf_len = 0;
  }

  Vector_push(sb->_body, strdup(string));
  sb->len += strlen(string);
}

void StringBuffer_appendChar(StringBuffer *sb, char c) {
  if (sb->_buf_len + 1 >= sb->_buf_siz) {
    sb->_buf[sb->_buf_len] = '\0';
    Vector_push(sb->_body, strdup(sb->_buf));
    sb->_buf_len = 0;
  }

  sb->_buf[sb->_buf_len] = c;
  sb->_buf_len++;
  sb->len++;
}

char *StringBuffer_toString(StringBuffer *sb) {
  char *str = calloc(sb->len + 1, sizeof(char));

  for (int i = 0; i < sb->_body->len; i++)
    strcat(str, sb->_body->data[i]);

  sb->_buf[sb->_buf_len] = '\0';
  strcat(str, sb->_buf);

  return str;
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
