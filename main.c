#include "httpd.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  Option *opt = parse(argc, argv);
  server_start(opt);

  return 0;
}

Option *parse(int argvc, char **argv) {
  Option *opts = malloc(sizeof(Option));
  opts->port = 8088;

  return opts;
}

void server_start(Option *opt) {
  Socket *sv_sock = create_server_socket(opt->port);
  printf("listen: %s:%d\n", inet_ntoa(sv_sock->addr->sin_addr),
         ntohs(sv_sock->addr->sin_port));

  while (true) {
    Socket *sock = server_accept(sv_sock);
    printf("address: %s, port: %d\n", inet_ntoa(sock->addr->sin_addr),
           ntohs(sock->addr->sin_port));

    pid_t pid = fork();
    switch (pid) {
    case -1: // error
      perror("fork");
      exit(1);
    case 0: // child
      worker_start(sock, opt);
      delete_socket(sock);
      _exit(0);
    default: // parent
             ;
    }
  }

  delete_socket(sv_sock);
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
