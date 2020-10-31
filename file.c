#include "file.h"
#include "util.h"

#include <dirent.h>    // opendir(3)
#include <stdlib.h>    // malloc(3)
#include <string.h>    // strdup(3)
#include <sys/stat.h>  // stat(2)
#include <sys/types.h> // stat(2)
#include <unistd.h>    // stat(2)

File *new_file2(char *parent_path, char *child_path) {
  File *file;
  char *path = malloc(strlen(parent_path) + strlen(child_path) + 1);
  strcpy(path, parent_path);
  strcat(path, child_path);

  file = new_file(path);
  free(path);

  return file;
}

File *new_file(char *path) {

  struct stat st;
  if (stat(path, &st) == -1) {
    return NULL;
  }

  File *file = malloc(sizeof(File));

  // File.path
  file->path = strdup(path);

  // File.ty
  switch (st.st_mode & S_IFMT) {
  case S_IFDIR:
    file->ty = F_DIR;
    break;
  case S_IFREG:
    file->ty = F_FILE;
    break;
  default:
    file->ty = F_OTHER;
  }

  // File.len
  file->len = st.st_size;

  return file;
}

char *parent_path(char *path) {
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

void delete_file(File *file) {
  if (file == NULL)
    return;

  free(file->path);
  free(file);
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

static void test_new_file() {
  File *file = new_file("LICENSE");
  expect(__LINE__, 1064, file->len);
  expect(__LINE__, F_FILE, file->ty);
  expect_str(__LINE__, "LICENSE", file->path);
  delete_file(file);

  file = new_file("LICENSE.");
  expect_ptr(__LINE__, NULL, file);
}

void run_all_test_file() {
  test_parent_path();
  test_filename();
  test_new_file();
}
