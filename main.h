#pragma once

#include "util.h"

#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME "Dali/0.1"
#define DEFAULT_PORT 8088

typedef struct {
  char *prog_name;
  bool debug;
  bool test;
  char *document_root;
  char *access_log;
  int port;
} Option;

void server_start(Option *);

extern Map *Mime_map;
