#pragma once

#include "util.h"
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h> // FILE

/* general net lib */
typedef struct {
  struct sockaddr_in *addr;
  socklen_t addr_len;
  int fd;
} Socket;

Socket *new_socket();
void delete_socket(Socket *);

Socket *create_server_socket(int);
Socket *server_accept(Socket *);

/* http lib */
typedef struct {
  char *method;
  char *request_uri;
  char *http_version;
  Map *header_map;
} HttpRequest;

typedef struct {
  char *http_version;
  char *status_code;
  char *reason_phrase;
  Map *header_map;
  char *body;
} HttpResponse;

HttpRequest *http_request_parse(int, bool);
HttpResponse *create_http_response(HttpRequest *);
void write_http_response(int, HttpResponse *);
void write_log(FILE *, Socket *, HttpRequest *, HttpResponse *);
