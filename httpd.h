#pragma once

#include <stdbool.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

typedef struct {
  struct sockaddr_in *addr;
  socklen_t addr_len;
  int fd;
} Socket;

typedef struct {
  bool debug;
  int port;
} Option;

Option *parse(int, char **);

void server_start(Option *);
void worker_start(Socket *, Option *);

Socket *create_server_socket(int);
Socket *server_accept(Socket *);

Socket *new_socket();
void delete_socket(Socket *);
