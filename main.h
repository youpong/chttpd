#pragma once

#include "net.h"

typedef struct {
  bool debug;
  int port;
} Option;

Option *parse(int, char **);

void server_start(Option *);
void worker_start(Socket *, Option *);
