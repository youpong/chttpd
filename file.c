#include "file.h"
#include "util.h"

#include <dirent.h> // readdir(3)
#include <stdlib.h> // malloc(3)
#include <string.h> // strdup(3)

// TODO: publish ?
char *parent_path(char *path);
char *filename(char *path);

File *open_file(char *path, char *mode) {
  File *file = malloc(sizeof(File));

  file->path = strdup(path);

  //
  // set file type
  //

  struct dirent *ent;
  char *dir = parent_path(path);
  if (strcmp(dir, "") == 0)
    dir = ".";
  DIR *d = opendir(dir);
  // TODO: catch error

  while ((ent = readdir(d)) != NULL) {
    if (strcmp(ent->d_name, filename(path)) == 0)
      break;
  }

  if (ent != NULL) {
    switch (ent->d_type) {
    case DT_DIR:
      file->ty = F_DIR;
    case DT_REG:
      file->ty = F_FILE;
    default:
      file->ty = F_OTHER;
    }
    file->len = ent->d_reclen;
  }

  file->f = fopen(path, mode);

  return file;
}

char *parent_path(char *path) {
  // TODO: impl
  char *ret = strdup(path);
  char *p;
  for (p = ret + strlen(ret); p >= ret; p--) {
    if (*p == '/') {
      *p = '\0';
      return ret;
    }
  }

  *ret = '\0';
  return ret;
}

char *filename(char *path) {
  char *p;
  for (p = path + strlen(path); p >= path; p--) {
    if (*p == '/')
      break;
  }

  return strdup(p + 1);
}

void close_file(File *file) {
}

void test_parent_path() {
  // absolute path
  expect_str(__LINE__, "/java/net", parent_path("/java/net/URL.java"));
  expect_str(__LINE__, "/java", parent_path("/java/net"));
  expect_str(__LINE__, "", parent_path("/java"));

  // relative path
  expect_str(__LINE__, "www", parent_path("www/index.html"));
  expect_str(__LINE__, "", parent_path("www"));
  expect_str(__LINE__, "", parent_path(""));
}

void test_filename() {
  expect_str(__LINE__, "URL.java", filename("/java/net/URL.java"));
  expect_str(__LINE__, "index.html", filename("index.html"));
  expect_str(__LINE__, "", filename(""));
}

static void test_open_file() {
  File *file = open_file("file.c", "r");
  expect(__LINE__, 15, file->len);
  expect(__LINE__, F_FILE, file->ty);
}

void run_all_test_file() {
  test_parent_path();
  test_filename();
  test_open_file();
}
