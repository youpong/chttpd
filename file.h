/**
 * @file file.h
 */
#pragma once

/// file type
typedef enum {
    F_DIR,   ///< directory
    F_FILE,  ///< regular file
    F_OTHER, ///< other
} FileType;

typedef struct {
    FileType ty;
    char *path;
    int len;
} File;

File *new_File(const char *path);
void delete_File(File *file);

char *parent_path(const char *path);
char *filename(const char *path);
char *extension(const char *path);

void run_all_test_file();
