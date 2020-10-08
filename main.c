#include "main.h"

#include <arpa/inet.h>  // inet_ntoa()
#include <netinet/in.h> // inet_ntoa()
#include <sys/socket.h> // inet_ntoa()

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
