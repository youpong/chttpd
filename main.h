#pragma once

#include "net.h"

#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME "Dali/0.1"

typedef struct {
  bool debug;
  bool test;
  char *document_root;
  int port;
} Option;

void server_start(Option *);
