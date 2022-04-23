/**
 * @file util.c
 */
#include "util.h"

#include <stdarg.h> // va_start(3)
#include <stdio.h>  // fprintf(3)
#include <stdlib.h> // free(3)
#include <string.h> // strcmp(3)

char *ErrorMsg;

//
// ArgsIter
//

/**
 * Creates a new ArgsIter object
 *
 * @return a pointer to a new ArgsIter object
 * @param argc the count of the arguments
 * @param argv the vector of the arguments
 */
ArgsIter *new_ArgsIter(int argc, char **argv) {
    ArgsIter *self = malloc(sizeof(ArgsIter));
    self->argc = argc;
    self->argv = argv;
    return self;
}

/**
 * Deletes the ArgsIter object
 *
 * @param self a pointer to ArgsIter object
 */
void delete_ArgsIter(ArgsIter *self) {
    free(self);
}

/**
 * Returns true if the Args has a next argument.
 *
 * @return true if the Args has a next argument.
 * @param self a pointer to ArgsIter object
 */
bool ArgsIter_hasNext(ArgsIter *self) {
    return self->argc > 0;
}

/**
 * Returns the next argument
 *
 * @return the next argument
 * @param self a pointer to ArgsIter object
 */
char *ArgsIter_next(ArgsIter *self) {
    char *ret = *(self->argv);
    self->argc--;
    self->argv++;

    return ret;
}

//
// Vector
//

/**
 * Creates a new Vector object
 *
 * @return a pointer to a new Vector object
 */
Vector *new_Vector() {
    Vector *vec = calloc(1, sizeof(Vector));
    vec->capacity = 16;
    vec->data = calloc(vec->capacity, sizeof(void *));
    vec->len = 0;
    return vec;
}

/**
 * Deletes the Vector object
 *
 * @param vec
 */
void delete_Vector(Vector *vec) {
    for (int i = 0; i < vec->len; i++)
        free(vec->data[i]);
    free(vec->data);
    free(vec);
}

/**
 * Pushes the element to the tail of the Vector.
 * automatically expand the Vector if necessary.
 *
 * @param vec
 * @param elem
 */
void Vector_push(Vector *vec, void *elem) {
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

/**
 * Pops the element from the tail of the vector.
 *
 * @param vec
 */
void *Vector_pop(Vector *vec) {
    return vec->data[--vec->len];
}

/**
 * Returns the element from tail of the Vector.
 *
 * @return the element from tail of the Vector.
 * @param vec
 */
void *Vector_last(Vector *vec) {
    return vec->data[vec->len - 1];
}

//
// Map
//

/**
 * Creates a new Map object
 *
 * @return a pointer to a new Map object
 */
Map *new_Map() {
    Map *map = malloc(sizeof(Map));
    map->keys = new_Vector();
    map->vals = new_Vector();
    return map;
}

/**
 * Destroys the Map object
 *
 * @param map
 */
void delete_Map(Map *map) {
    delete_Vector(map->vals);
    delete_Vector(map->keys);
    free(map);
}

/**
 * Associates the value with the key in this map
 *
 * @param map
 * @param key
 * @param val
 */
void Map_put(Map *map, char *key, void *val) {
    Vector_push(map->keys, key);
    Vector_push(map->vals, val);
}

/**
 * Returns the value to which the key is mapped, or NULL if the map contains no
 * mapping for the key.
 *
 * @return the value to witch the key is mapped, or NULL if the map contains no
 * mapping for the key.
 * @param map
 * @param key
 */
void *Map_get(Map *map, const char *key) {
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

/**
 * Creates a new StringBuffer object.
 *
 * @return a pointer to a new StringBuffer object
 */
StringBuffer *new_StringBuffer() {
    StringBuffer *sb = calloc(1, sizeof(StringBuffer));

    sb->len = 0;
    sb->_body = new_Vector();
    sb->_buf_siz = 255 + 1;
    sb->_buf = calloc(sb->_buf_siz, sizeof(char));

    return sb;
}

/**
 * Destroys the StringBuffer object.
 *
 * @param sb
 */
void delete_StringBuffer(StringBuffer *sb) {
    delete_Vector(sb->_body);
    free(sb->_buf);
    free(sb);
}

/**
 * Appends the string to StringBuffer.
 *
 * @param sb
 * @param string
 */
void StringBuffer_append(StringBuffer *sb, const char *string) {
    if (sb->_buf_len != 0) {
        sb->_buf[sb->_buf_len] = '\0';
        Vector_push(sb->_body, strdup(sb->_buf));
        sb->_buf_len = 0;
    }

    Vector_push(sb->_body, strdup(string));
    sb->len += strlen(string);
}

/**
 * Appends the char to StringBuffer.
 *
 * @param sb
 * @param c
 */
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

/**
 * Creates string by StringBuffer
 *
 * Caller must free the allocated memory stores the string.
 *
 * @return a string
 * @param sb
 */
char *StringBuffer_toString(StringBuffer *sb) {
    char *str = calloc(sb->len + 1, sizeof(char));

    for (int i = 0; i < sb->_body->len; i++)
        strcat(str, sb->_body->data[i]);

    sb->_buf[sb->_buf_len] = '\0';
    strcat(str, sb->_buf);

    return str;
}

/**
 * duplicate a integer
 *
 * @return a pointer to the duplicated integer
 * @param n
 */
int *intdup(int n) {
    int *num = malloc(sizeof(int));
    *num = n;
    return num;
}

//
// for testing
//

/**
 * put a error message to stderr, then exit with failure status code
 *
 * @parma fmt
 */
noreturn void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

/**
 * Assert the expected and actual are equal integer.
 *
 * @param line
 * @param expected
 * @param actual
 */
void expect(int line, int expected, int actual) {
    if (expected == actual)
        return;
    error("%d: %d expected, but got %d", line, expected, actual);
}

/**
 * Assert the extected and actual are equal string.
 *
 * @param line
 * @param expected
 * @param actual
 */
void expect_str(int line, const char *expected, const char *actual) {
    if (expected == NULL)
        error("%d: non-NULL is expected, but \"expected\" is NULL", line);
    if (actual == NULL)
        error("%d: non-NULL is expected, but \"actual\" is NULL", line);
    if (strcmp(expected, actual) != 0)
        error("%d: \"%s\" expected, but got \"%s\"", line, expected, actual);
    return;
}

/**
 * Assert the expected and actual point are equal.
 *
 * @param line
 * @param expected
 * @param actual
 */
void expect_ptr(int line, const void *expected, const void *actual) {
    if (expected == actual)
        return;
    error("%d: %d expected, but got %d", line, expected, actual);
}

/**
 * Assert the expected and the actual are equal boolean value
 *
 * @param line
 * @param expected
 * @param actual
 */
void expect_bool(int line, bool expected, bool actual) {
    if (expected == actual)
        return;
    if (actual == true)
        error("%d: 'false' expected, but got 'true'", line);
    else
        error("%d: 'true' expected, but got 'false'", line);
}
