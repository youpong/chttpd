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

typedef struct {
  char *peer_addr;
  time_t date;
} Logger;

Option *parse(int, char **);
void server_start(Option *);
Socket *create_server_socket(int);
Socket *server_accept(Socket *);
Socket *new_socket();
void socket_close(Socket *);
//void delete_socket(Socket *);
void worker_start(int, struct sockaddr_in *, Option *);
