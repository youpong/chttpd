#include "net.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

Socket *new_socket() {
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

HttpRequest *http_request_parse(int fd, bool debug) {
  HttpRequest *req = malloc(sizeof(HttpRequest));
  req->header_map = new_map();

  FILE *f = fdopen(fd, "r");

  return req;
}

HttpResponse *create_http_response(HttpRequest *req) {
  return NULL;
}

void write_http_response(int fd, HttpResponse *res) {
}

void write_log(FILE *out, Socket *sock, HttpRequest *req, HttpResponse *res) {
}

