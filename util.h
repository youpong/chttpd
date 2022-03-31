// -*- mode: c -*-
/**
 * @file util.h
 */
#pragma once

#include <stdbool.h>     // bool
#include <stdnoreturn.h> // noreturn

/* util.c */
typedef enum {
    E_Okay,
    E_Failure,
    HM_EmptyRequest,
    HM_BadRequest,
    O_IllegalArgument,
} ExceptionType;

typedef struct {
    ExceptionType ty;
    char *msg;
} Exception;

/// Iterator.
typedef struct {
    int argc;
    char **argv;
} ArgsIter;

ArgsIter *new_ArgsIter(int, char **);
void delete_ArgsIter(ArgsIter *);
bool ArgsIter_hasNext(ArgsIter *);
char *ArgsIter_next(ArgsIter *);

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_Vector();
void delete_Vector(Vector *);
void Vector_push(Vector *, void *);
void *Vector_pop(Vector *);
void *Vector_last(Vector *);

typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *new_Map();
void delete_Map(Map *);
void Map_put(Map *, char *, void *);
/**
 * @return value
 * @return NULL key is not found
 */
void *Map_get(Map *, char *);

typedef struct {
    int len;
    Vector *_body;

    char *_buf;
    int _buf_len;
    int _buf_siz;
} StringBuffer;

StringBuffer *new_StringBuffer();
void delete_StringBuffer(StringBuffer *);
void StringBuffer_append(StringBuffer *sb, char *string);
void StringBuffer_appendChar(StringBuffer *sb, char c);
char *StringBuffer_toString(StringBuffer *);

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
