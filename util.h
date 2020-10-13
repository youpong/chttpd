#pragma once

#include <stdnoreturn.h>
#include <stdbool.h>

/* util.c */
typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector();
void vec_push(Vector *, void *);
void *vec_pop(Vector *);
void *vec_last(Vector *);

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

Map *new_map();
void map_put(Map *, char *, void *);
void *map_get(Map *, char *);

int *intdup(int);

noreturn void error(char *, ...);

// test
// __LINE__ 
void expect(int line, int expected, int actual);
void expect_str(int line, char *expected, char *actual);
void expect_ptr(int line, void *expected, void *actual);
void expect_bool(int line, bool expected, bool actual);
  
/* util_test.c */
void run_utiltest();

