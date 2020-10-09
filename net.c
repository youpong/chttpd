#include "net.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h> // strdup(3)

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
  addr->sin_port = htons(8088);
  addr->sin_addr.s_addr = INADDR_ANY;
  if (bind(sv_sock->fd, (struct sockaddr *)addr, sv_sock->addr_len) < 0) {
    perror("bind");
    exit(1);
  }

  /* listen */
  if (listen(sv_sock->fd, 5) == -1) {
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
static void request_line(FILE *f, HttpRequest *req);
static void message_header(FILE *f, HttpRequest *req);

/**
 * @return a pointer to HttpRequest object.
 * @return NULL when read null request.
 */
HttpRequest *http_request_parse(int fd, bool debug) {
  HttpRequest *req = malloc(sizeof(HttpRequest));
  req->header_map = new_map();

  FILE *f = fdopen(fd, "r");

  request_line(f, req);
  message_header(f, req);

  return req;
}

static void request_line(FILE *f, HttpRequest *req) {
  char buf[256];
  char *p;
  int c;

  p = buf;
  while ((c = fgetc(f)) != EOF) {
    if (c == ' ')
      break;
    *p++ = c;
  }
  *p = '\0';
  req->method = strdup(buf);

  p = buf;
  while ((c = fgetc(f)) != EOF) {
    if (c == ' ')
      break;
    *p++ = c;
  }
  *p = '\0';
  req->request_uri = strdup(buf);

  p = buf;
  while ((c = fgetc(f)) != EOF) {
    if (c == '\r') {
      consum(f, '\n');
      break;
    }
    *p++ = c;
  }
  *p = '\0';
  req->http_version = strdup(buf);
}

static void message_header(FILE *f, HttpRequest *req) {
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

    map_put(req->header_map, key, value);
  }
}

static void consum(FILE *f, char expected) {
  int c;

  c = fgetc(f);
  if (c != expected)
    error("unexpected character: %c\n", c);
}

void write_http_response(int fd, HttpResponse *res) {

  FILE *f = fdopen(fd, "w");

  // status_line
  fprintf(f, "%s %s %s\r\n", res->http_version, res->status_code,
          res->reason_phrase);

  // headers
  Map *map = res->header_map;
  for (int i = 0; i < map->keys->len; i++) {
    fprintf(f, "%s: %s\r\n", (char *)map->keys->data[i],
            (char *)map->vals->data[i]);
  }

  fprintf(f, "\r\n");

  // body
  char *p = res->body;
  while (*p != '\0') {
    fputc(*p, f);
    p++;
  }
  fflush(f);
}

void write_log(FILE *out, Socket *sock, HttpRequest *req, HttpResponse *res) {
}
