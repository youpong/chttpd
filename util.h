// -*- mode: c -*-
/** @file
 * provides utility functions and interface to container, error and test.
 *
 * interfaces
 * \li error - interface to manipulate errors.
 * \li test - interface for automated testing.
 *
 * Containers
 * \li ArgsIter - an iterator for arguments
 * \li Vector - an ordered collection.
 * \li Map - an object that maps keys to values.
 * \li StringBuffer - mutable sequence of characters.
 *
 * Functions
 * \li intdup - duplicate an integer
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
void *Map_get(Map *, const char *);

typedef struct {
    int len;
    Vector *_body;

    char *_buf;
    int _buf_len;
    int _buf_siz;
} StringBuffer;

StringBuffer *new_StringBuffer();
void delete_StringBuffer(StringBuffer *);
void StringBuffer_append(StringBuffer *sb, const char *string);
void StringBuffer_appendChar(StringBuffer *sb, char c);
char *StringBuffer_toString(StringBuffer *);

int *intdup(int);

noreturn void error(char *, ...);

// test
// __LINE__
void expect(int line, int expected, int actual);
void expect_str(int line, const char *expected, const char *actual);
void expect_ptr(int line, const void *expected, const void *actual);
void expect_bool(int line, bool expected, bool actual);

/* util_test.c */
void run_all_test_util();
