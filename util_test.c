#include "util.h"

#include <stdio.h>  // fopen(3)
#include <stdlib.h> // free(3)
#include <string.h> // strdup(3)

static void test_ArgsIter() {
    char *argv_0[] = {"prog-name"};
    ArgsIter *iter = new_ArgsIter(1, argv_0);

    expect_str(__LINE__, "prog-name", iter->prog_name);
    expect(__LINE__, false, ArgsIter_hasNext(iter));
    delete_ArgsIter(iter);

    char *argv_1[] = {"prog-name", "-h"};
    iter = new_ArgsIter(2, argv_1);

    expect_str(__LINE__, "prog-name", iter->prog_name);
    expect(__LINE__, true, ArgsIter_hasNext(iter));
    expect_str(__LINE__, "-h", ArgsIter_next(iter));
    expect(__LINE__, false, ArgsIter_hasNext(iter));
    delete_ArgsIter(iter);
}

static void test_Vector() {
    Vector *vec = new_Vector();

    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 100; i++) {
        Vector_push(vec, intdup(i));
    }

    // clang-format off
    expect(__LINE__, 100, vec->len);
    expect(__LINE__,   0, *(int *)vec->data[ 0]);
    expect(__LINE__,  50, *(int *)vec->data[50]);
    expect(__LINE__,  99, *(int *)vec->data[99]);
    expect(__LINE__,  99, *(int *)Vector_last(vec));

    expect(__LINE__,  99, *(int *)Vector_pop(vec));  
    expect(__LINE__,  98, *(int *)Vector_last(vec));
    expect(__LINE__,  99, vec->len);
    // clang-format on

    delete_Vector(vec);
}

static void test_Map() {
    Map *map = new_Map();

    expect_ptr(__LINE__, NULL, Map_get(map, "foo"));

    Map_put(map, strdup("foo"), intdup(2));
    expect(__LINE__, 2, *(int *)Map_get(map, "foo"));

    Map_put(map, strdup("bar"), intdup(4));
    expect(__LINE__, 4, *(int *)Map_get(map, "bar"));

    Map_put(map, strdup("foo"), intdup(6));
    expect(__LINE__, 6, *(int *)Map_get(map, "foo"));

    // put empty string at key
    expect_ptr(__LINE__, NULL, Map_get(map, ""));
    Map_put(map, strdup(""), strdup("value"));
    expect_str(__LINE__, "value", Map_get(map, ""));

    delete_Map(map);
}

static void test_StringBuffer() {
    char *str;
    StringBuffer *sb;

    //
    // 1.
    //
    sb = new_StringBuffer();
    StringBuffer_append(sb, "One");
    StringBuffer_append(sb, "Two");
    str = StringBuffer_toString(sb);
    expect_str(__LINE__, "OneTwo", str);
    free(str);

    StringBuffer_appendChar(sb, '3');
    str = StringBuffer_toString(sb);
    expect_str(__LINE__, "OneTwo3", str);
    free(str);

    StringBuffer_append(sb, "Four");
    str = StringBuffer_toString(sb);
    expect_str(__LINE__, "OneTwo3Four", str);
    free(str);

    delete_StringBuffer(sb);

    //
    // 2.
    //
    sb = new_StringBuffer();
    FILE *f = fopen("LICENSE", "r");
    int c;
    while ((c = fgetc(f)) != EOF) {
        StringBuffer_appendChar(sb, c);
    }
    fclose(f);

    str = StringBuffer_toString(sb);
    expect(__LINE__, 1064, strlen(str));

    free(str);
    delete_StringBuffer(sb);
}

static void test_strcmp() {
    //
    // compare empty string
    //
    // clang-format off
    expect(__LINE__, true,  strcmp("",  "") == 0);
    expect(__LINE__, false, strcmp("A", "") == 0);
    expect(__LINE__, false, strcmp("", "A") == 0);
    // clang-format on
}

static void test_sizeof() {
    char *buf = malloc(256);
    expect(__LINE__, 8, sizeof(buf));
    free(buf);
}

void run_all_test_util() {
    test_ArgsIter();
    test_Vector();
    test_Map();
    test_StringBuffer();
    test_strcmp();
    test_sizeof();
}
