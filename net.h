#pragma once

#include "util.h"

#include <netinet/in.h> // socklen_t
#include <stdbool.h>    // bool
#include <stdio.h>      // FILE

/* general net lib */

#define LISTEN_QUEUE 6

typedef enum {
  S_SRV, // server socket
  S_CLT, // client socket
} SocketType;

typedef struct {
  SocketType _ty; // for internal: type of socket(Server/Client)

  struct sockaddr_in *addr;
  socklen_t addr_len;

  int _fd; // for internal: file descriptor

  FILE *ips; // Input Stream
  FILE *ops; // Output Stream
} Socket;

Socket *create_server_socket(int);
Socket *server_accept(Socket *);
void delete_socket(Socket *);

void url_decode(char *dest, char *src);

/* http lib */
typedef enum {
  HM_REQ, // Http Request
  HM_RES, // Http Response
} HttpMessageType;

/**
 * HTTP-message    = Request | Response
 *
 * generic-message = start-line
 *                   *(message-header CRLF)
 *                   [ message-body ]
 * start-line      = Request-Line | Status-Line
 *
 * Request-Line = Method       SP Request-URI SP HTTP-Version  CRLF
 * Status-Line  = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
 *
 */
typedef struct {
  HttpMessageType _ty; // for internal: type of message(request/response)

  // start-line (Request-Line|Status-Line)
  char *method;
  char *request_uri;
  char *http_version;
  char *status_code;
  char *reason_phrase;

  // message-header
  Map *header_map;

  // message-body
  char *body;
  int body_len;

  // utilities
  char *filename; // pick out from request_uri

} HttpMessage;

HttpMessage *new_HttpMessage(HttpMessageType ty);
void delete_HttpMessage(HttpMessage *);
HttpMessage *HttpMessage_parse(FILE *, HttpMessageType, bool);
void write_http_message(FILE *, HttpMessage *);

void run_all_test_net();
