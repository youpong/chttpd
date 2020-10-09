#pragma once

#include "net.h"

#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME "Dali"

typedef struct {
  bool debug;
  bool test;
  int port;
} Option;

Option *parse(int, char **);

void server_start(Option *);
void worker_start(Socket *, FILE *, Option *);
