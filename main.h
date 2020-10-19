#pragma once

#include "net.h"

#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME "Dali/0.1"
#define DEFAULT_PORT 8088

typedef struct {
  char *prog_name;
  bool debug;
  bool test;
  char *document_root;
  int port;
} Option;

void server_start(Option *);
