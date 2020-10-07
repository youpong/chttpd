#include "httpd.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

/*
typedef struct {
} HttpMessage;
*/

HttpRequest *HttpRequestParse(int, bool);
HttpResponse *create_http_response(HttpRequest *);
void write_http_response(int, HttpResponse *);
void write_log(FILE *, Socket *, HttpRequest *, HttpResponse *);

void worker_start(Socket *sock, Option *opt) {

  while (true) {
    HttpRequest *req = HttpRequestParse(sock->fd, opt->debug);
    HttpResponse *res = create_http_response(req);
    write_http_response(sock->fd, res);
    write_log(stdout, sock, req, res);
    write(sock->fd, "Hello", 5); // for debug

    //    if (strcmp("close", get_header(req->header, "Connection")) == 0)
    break;
  }
}

HttpRequest *HttpRequestParse(int fd, bool debug) {
  return NULL;
}

HttpResponse *create_http_response(HttpRequest *req) {
  return NULL;
}

void write_http_response(int fd, HttpResponse *res) {
}

void write_log(FILE *out, Socket *sock, HttpRequest *req, HttpResponse *res) {
}
