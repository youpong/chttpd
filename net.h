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

Socket *create_server_socket(int);
Socket *server_accept(Socket *);
void delete_socket(Socket *);

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

typedef struct {
  /* Http Request */
  /* Request Line: method request_uir http_version */
  /* Http REsponse */
  /* Status Line: http_version status_code reason_phrase */
  char *method;
  char *request_uri;
  char *http_version;
  char *status_code;
  char *reason_phrase;

  Map *header_map;

  char *body;
} HttpMessage;

HttpRequest *http_request_parse(int, bool);
void write_http_response(int, HttpResponse *);

