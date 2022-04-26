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

/** @struct Exception
 */
typedef struct {
    ExceptionType ty;
    char *msg;
} Exception;

/** @struct ArgsIter
 * @brief An iterator for arguments.
 *
 * \li new_ArgsIter()
 * \li delete_ArgsIter()
 * \li ArgsIter_hasNext()
 * \li ArgsIter_next()
 *
 * @example args_iter.c
 */
typedef struct {
    char *prog_name;
    int argc;
    char **argv;
} ArgsIter;

ArgsIter *new_ArgsIter(int, char **);
void delete_ArgsIter(ArgsIter *);
char *ArgsIter_getProgName(ArgsIter *);
bool ArgsIter_hasNext(ArgsIter *);
char *ArgsIter_next(ArgsIter *);

/** @struct Vector
 *
 * \li new_Vector();
 * \li delete_Vector()
 * \li Vector_push()
 * \li Vector_pop()
 * \li Vector_last()
 */
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

/** @struct Map
 *
 * \li new_Map()
 * \li delete_Map()
 * \li Map_put()
 * \li Map_get()
 */
typedef struct {
    Vector *keys;
    Vector *vals;
} Map;

Map *new_Map();
void delete_Map(Map *);
void Map_put(Map *, char *, void *);
void *Map_get(Map *, const char *);

/** @struct StringBuffer
 *
 * new_StringBuffer()
 * delete_StringBuffer()
 * StringBuffer_append()
 * StringBuffer_appendChar()
 * StringBuffer_toString()
 */
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

//
// test
//
void expect(int line, int expected, int actual);
void expect_str(int line, const char *expected, const char *actual);
void expect_ptr(int line, const void *expected, const void *actual);
void expect_bool(int line, bool expected, bool actual);

/* util_test.c */
void run_all_test_util();
