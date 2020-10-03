#pragma once

#include <stdbool.h>
#include <time.h>

typedef struct {
  bool debug;
} Option;

typedef struct {
  char *peer_addr;
  time_t date;
} Logger;

Option *parse(int, char **);
void server_start(Option *);
int sv_listen(Option *);
int sv_accept(int);
void worker_start(int, Option *);
