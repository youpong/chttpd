#include "net.h"
#include "util.h"

#include <assert.h>
#include <fcntl.h> // open(2)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>    // strdup(3)
#include <sys/stat.h>  // oepn(2)
#include <sys/types.h> // open(2)
#include <unistd.h>

static Socket *new_socket() {
  Socket *sock = malloc(sizeof(Socket));
  sock->addr = malloc(sizeof(struct sockaddr_in));
  sock->addr_len = sizeof(*sock->addr);

  return sock;
}

void delete_socket(Socket *sock) {
  close(sock->fd);
  free(sock->addr);
  free(sock);
}

Socket *create_server_socket(int port) {
  Socket *sv_sock = new_socket();

  /* create a socket, endpoint of connection */
  if ((sv_sock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  /* bind */
  struct sockaddr_in *addr = sv_sock->addr;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = INADDR_ANY;
  if (bind(sv_sock->fd, (struct sockaddr *)addr, sv_sock->addr_len) < 0) {
    perror("bind");
    exit(1);
  }

  /* listen */
  if (listen(sv_sock->fd, LISTEN_QUEUE) == -1) {
    perror("listen");
    exit(1);
  }

  return sv_sock;
}

Socket *server_accept(Socket *sv_sock) {
  Socket *sock = new_socket();

  if ((sock->fd = accept(sv_sock->fd, (struct sockaddr *)sock->addr,
                         &sock->addr_len)) < 0) {
    perror("accept");
    exit(1);
  }

  return sock;
}

static void consum(FILE *f, char c);
static void request_line(FILE *f, HttpMessage *req);
static void message_header(FILE *f, HttpMessage *req);

HttpMessage *new_HttpMessage(int ty) {
  HttpMessage *result = calloc(1, sizeof(HttpMessage));
  result->ty = ty;
  result->header_map = new_map();
  return result;
}

void delete_HttpMessage(HttpMessage *msg) {
  switch (msg->ty) {
  case HM_REQ:
    free(msg->method);
    free(msg->request_uri);
    free(msg->http_version);
    break;
  case HM_RES:
    free(msg->http_version);
    free(msg->status_code);
    free(msg->reason_phrase);
    break;
  default:
    error("Unknown HttpMessage type: %d", msg->ty);
  }

  delete_map(msg->header_map);

  if (msg->body != NULL)
    free(msg->body);

  free(msg);
}

/**
 * @return a pointer to HttpRequest object.
 * @return NULL when read null request.
 */
HttpMessage *http_message_parse(int fd, int ty, bool debug) {
  assert(ty == HM_REQ); // not implemented HM_RES yet.
  int c;

  FILE *f = fdopen(fd, "r");
  if (f == NULL) {
    perror("fdopen");
    exit(1);
  }

  if ((c = fgetc(f)) == EOF)
    return NULL;
  ungetc(c, f);

  HttpMessage *msg = new_HttpMessage(ty);
  switch (ty) {
  case HM_REQ:
    request_line(f, msg);
    break;
  case HM_RES:
    // TODO: implement
    // status_line(f, msg);
    break;
  }

  message_header(f, msg);
  // message_body(f, msg);

  return msg;
}

static void request_line(FILE *f, HttpMessage *msg) {
  char buf[256];
  char *p;
  int c;

  assert(msg->ty == HM_REQ);

  p = buf;
  while ((c = fgetc(f)) != EOF) {
    if (c == ' ')
      break;
    *p++ = c;
  }
  *p = '\0';
  msg->method = strdup(buf);

  p = buf;
  while ((c = fgetc(f)) != EOF) {
    if (c == ' ')
      break;
    *p++ = c;
  }
  *p = '\0';
  msg->request_uri = strdup(buf);

  p = buf;
  while ((c = fgetc(f)) != EOF) {
    if (c == '\r') {
      consum(f, '\n');
      break;
    }
    *p++ = c;
  }
  *p = '\0';
  msg->http_version = strdup(buf);
}

static void message_header(FILE *f, HttpMessage *msg) {
  char buf[256];
  char *p, *key, *value;
  int c;

  while ((c = fgetc(f)) != EOF) {
    if (c == '\r') {
      consum(f, '\n');
      break;
    }
    ungetc(c, f);

    // key
    p = buf;
    while ((c = fgetc(f)) != EOF) {
      if (c == ':')
        break;
      *p++ = c;
    }
    *p = '\0';
    key = strdup(buf);

    // SP
    consum(f, ' ');

    // value
    p = buf;
    while ((c = fgetc(f)) != EOF) {
      if (c == '\r') {
        consum(f, '\n');
        break;
      }
      *p++ = c;
    }
    *p = '\0';
    value = strdup(buf);

    map_put(msg->header_map, key, value);
  }
}

static void consum(FILE *f, char expected) {
  int c;

  c = fgetc(f);
  if (c != expected)
    error("unexpected character: %c\n", c);
}

void write_http_message(int fd, HttpMessage *msg) {
  assert(msg->ty == HM_RES); // HM_REQ not implemented yet.

  FILE *f = fdopen(fd, "w");

  switch (msg->ty) {
  case HM_REQ:
    break;
  case HM_RES:
    // status_line
    fprintf(f, "%s %s %s\r\n", msg->http_version, msg->status_code,
            msg->reason_phrase);
  }

  // headers
  Map *map = msg->header_map;
  for (int i = 0; i < map->keys->len; i++) {
    fprintf(f, "%s: %s\r\n", (char *)map->keys->data[i],
            (char *)map->vals->data[i]);
  }

  fprintf(f, "\r\n");

  // body
  char *p = msg->body;
  while (*p != '\0') {
    fputc(*p, f);
    p++;
  }
  fflush(f);
}

void test_http_message_parse() {
  char tmp_file[] = "XXXXXX";
  int fd = mkstemp(tmp_file);
  FILE *f = fdopen(fd, "w");
  fprintf(f, "GET /hello.html HTTP/1.1\r\n");
  fprintf(f, "Host: localhost\r\n");
  fprintf(f, "\r\n");
  fclose(f);

  fd = open(tmp_file, O_RDONLY);
  HttpMessage *req = http_message_parse(fd, HM_REQ, false);

  expect(__LINE__, HM_REQ, req->ty);
  expect_str(__LINE__, "GET", req->method);
  expect_str(__LINE__, "/hello.html", req->request_uri);
  expect_str(__LINE__, "HTTP/1.1", req->http_version);
  expect_str(__LINE__, "localhost", (char *)map_get(req->header_map, "Host"));

  close(fd);
  unlink(tmp_file);
}

void test_write_http_message() {
  HttpMessage *res = new_HttpMessage(HM_RES);

  res->http_version = strdup("HTTP/1.1");
  res->status_code = strdup("200");
  res->reason_phrase = strdup("OK");

  map_put(res->header_map, strdup("Server"), strdup("Dali/0.1"));

  res->body = strdup("body");

  char *template = strdup("XXXXXX");
  int fd = mkstemp(template);

  write_http_message(fd, res);

  char buf[1024];
  FILE *f = fopen(template, "r");
  char *p = buf;
  int c;
  while ((c = fgetc(f)) != EOF) {
    *p++ = c;
  }
  *p = '\0';
  fclose(f);

  expect_str(__LINE__,
             "HTTP/1.1 200 OK\r\n"
             "Server: Dali/0.1\r\n"
             "\r\n"
             "body",
             buf);
  unlink(template);

  delete_HttpMessage(res);
}
