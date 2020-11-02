#include "util.h"

#include <string.h> // strdup(3)

static void test_vector() {
  Vector *vec = new_vector();

  expect(__LINE__, 0, vec->len);

  for (int i = 0; i < 100; i++) {
    vec_push(vec, intdup(i));
  }

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, *(int *)vec->data[0]);
  expect(__LINE__, 50, *(int *)vec->data[50]);
  expect(__LINE__, 99, *(int *)vec->data[99]);
  expect(__LINE__, 99, *(int *)vec_last(vec));
}

static void test_map() {
  Map *map = new_map();

  expect_ptr(__LINE__, NULL, map_get(map, "foo"));

  map_put(map, strdup("foo"), intdup(2));
  expect(__LINE__, 2, *(int *)map_get(map, "foo"));

  map_put(map, strdup("bar"), intdup(4));
  expect(__LINE__, 4, *(int *)map_get(map, "bar"));

  map_put(map, strdup("foo"), intdup(6));
  expect(__LINE__, 6, *(int *)map_get(map, "foo"));

  // put empty string at key
  expect_ptr(__LINE__, NULL, map_get(map, ""));
  map_put(map, strdup(""), strdup("value"));
  expect_str(__LINE__, "value", map_get(map, ""));

  delete_map(map);
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
  test_vector();
  test_map();
  test_strcmp();
}
