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
enum HttpMessage_type {
  HM_REQ, // Http Request		       
  HM_RES, // Http Response
};

/**
 * HttpMessage
 * - Http Request 
 *   Request Line: method request_uir http_version 
 * - Http REsponse 
 *   Status Line: http_version status_code reason_phrase 
 */
typedef struct {
  int ty; // type of http message(request/response)
  
  char *method;
  char *request_uri;
  char *http_version;
  char *status_code;
  char *reason_phrase;

  Map *header_map;

  char *body;
} HttpMessage;

HttpMessage *http_request_parse(int, bool);
void write_http_response(int, HttpMessage *);
