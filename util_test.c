#include "util.h"

#include <string.h> // strdup(3)

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

static void test_strcmp() {
  //
  // compare empty string
  //
  // clang-format off
  expect(__LINE__,  true, strcmp("",  "") == 0);
  expect(__LINE__, false, strcmp("A", "") == 0);
  expect(__LINE__, false, strcmp("", "A") == 0);
  // clang-format on
}

void run_all_test_util() {
  test_Vector();
  test_Map();
  test_strcmp();
}
