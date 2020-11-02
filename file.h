#pragma once

typedef enum {
  F_DIR,   // directory
  F_FILE,  // regular file
  F_OTHER, // other
} FileType;

typedef struct {
  FileType ty;
  char *path;
  int len;
} File;

File *new_File(char *path);
File *new_File2(char *parent_path, char *child_path);
void delete_File(File *file);

char *parent_path(char *path);
char *filename(char *path);
char *extension(char *path);

void run_all_test_file();
