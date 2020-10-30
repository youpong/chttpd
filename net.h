#pragma once

#include "util.h"
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h> // FILE

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
  
  int _fd;   // for internal: file descriptor
  
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
 * HttpMessage
 * - Http Request 
 *   Request Line: method request_uir http_version 
 * - Http REsponse 
 *   Status Line: http_version status_code reason_phrase 
 */
typedef struct {
  HttpMessageType _ty; // for internal: type of message(request/response)
  
  char *method;
  char *request_uri;
  char *http_version;
  char *status_code;
  char *reason_phrase;

  Map *header_map;

  char *body;
} HttpMessage;

HttpMessage *new_HttpMessage(HttpMessageType ty);
void delete_HttpMessage(HttpMessage *);
HttpMessage *http_message_parse(FILE *, HttpMessageType, bool);
void write_http_message(FILE *, HttpMessage *);

void run_all_test_net();
