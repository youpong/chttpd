#pragma once

#include "util.h"
#include <setjmp.h>

// clang-format off
#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME  "Dali/0.1"
#define DEFAULT_PORT 8088
#define MAX_SERVERS  5
// clang-format on

enum {
  EX_OK = 0,
  EX_ILLEGAL_ARG,
  EX_BAD_REQUEST,
};

typedef struct {
  char *prog_name;
  bool debug;
  bool test;
  char *document_root;
  char *access_log;
  int port;
} Option;

void server_start(Option *);
void run_all_test_server();

extern jmp_buf g_env;
extern Map *MimeMap;
