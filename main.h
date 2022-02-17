#pragma once

#include "util.h"

// clang-format off
#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME  "Dali"
#define DEFAULT_PORT 8088
#define MAX_SERVERS  5
// clang-format on

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

extern Map *MimeMap;
