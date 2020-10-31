#pragma once

#include <stdio.h>

typedef enum {
  F_DIR,  // directory 
  F_FILE, // regular file
  F_OTHER,// other
} FileType;

typedef struct {
  FileType ty;  
  //FILE *f;
  //DIR *d;
  char *path;
  int len;
} File;

File *new_file(char *path);
void delete_file(File *file);

void run_all_test_file();
