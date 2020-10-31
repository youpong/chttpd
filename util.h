// -*- mode: c -*-
#pragma once

#include <stdnoreturn.h> // noreturn
#include <stdbool.h> // bool

/* util.c */
typedef struct {
  int argc;
  char **argv;
} Args;

Args *new_args(int, char **);
//void delete_args(Args *);  
bool args_has_next(Args *);
char *args_next(Args *);

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector();
void delete_vector(Vector *);
void vec_push(Vector *, void *);
void *vec_pop(Vector *);
void *vec_last(Vector *);

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

Map *new_map();
void delete_map(Map *);
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
void run_all_test_util();
