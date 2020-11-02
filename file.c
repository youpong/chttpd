#include "file.h"
#include "util.h"

#include <dirent.h>    // opendir(3)
#include <stdlib.h>    // malloc(3)
#include <string.h>    // strdup(3)
#include <sys/stat.h>  // stat(2)
#include <sys/types.h> // stat(2)
#include <unistd.h>    // stat(2)

File *new_File2(char *parent_path, char *child_path) {
  File *file;
  char *path = malloc(strlen(parent_path) + strlen(child_path) + 1);
  strcpy(path, parent_path);
  strcat(path, child_path);

  file = new_File(path);
  free(path);

  return file;
}

File *new_File(char *path) {

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

void delete_File(File *file) {
  if (file == NULL)
    return;

  free(file->path);
  free(file);
}

// duplicate
char *parent_path(char *path) {
  char *ret = strdup(path);

  for (char *p = ret + strlen(ret); p >= ret; p--) {
    if (*p == '/') {
      *p = '\0';
      return ret;
    }
  }

  *ret = '\0';
  return ret;
}

// duplicate
char *filename(char *path) {
  char *p = strrchr(path, '/');
  if (p == NULL)
    return strdup(path);

  return strdup(p + 1);
}

// duplicate
char *extension(char *path) {
  char *fname = filename(path);
  char *p = strrchr(fname, '.');
  if (p == NULL)
    return NULL;

  return strdup(p + 1);
}

static void test_parent_path() {
  // absolute path
  expect_str(__LINE__, "/java/net", parent_path("/java/net/URL.java"));
  expect_str(__LINE__, "/java", parent_path("/java/net"));
  expect_str(__LINE__, "", parent_path("/java"));

  // relative path
  expect_str(__LINE__, "www", parent_path("www/index.html"));
  expect_str(__LINE__, "", parent_path("www"));
  expect_str(__LINE__, "", parent_path(""));
}

static void test_filename() {
  expect_str(__LINE__, "URL.java", filename("/java/net/URL.java"));
  expect_str(__LINE__, "index.html", filename("index.html"));
  expect_str(__LINE__, "", filename(""));
}

static void test_extension() {
  // clang-format off
  expect_str(__LINE__, "ext", extension("dir/foo.ext"));
  expect_str(__LINE__, "",    extension("dir/foo."));        
  expect_ptr(__LINE__, NULL,  extension("dir.name/foo"));
  // clang-format on
}

static void test_new_File() {
  File *file;

  // normal case
  file = new_File("LICENSE");
  expect(__LINE__, 1064, file->len);
  expect(__LINE__, F_FILE, file->ty);
  expect_str(__LINE__, "LICENSE", file->path);
  delete_File(file);

  // file not found
  file = new_File("LICENSE.");
  expect_ptr(__LINE__, NULL, file);
  delete_File(file);
}

void run_all_test_file() {
  test_parent_path();
  test_filename();
  test_extension();
  test_new_File();
}
