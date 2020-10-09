#include "main.h"

#include <arpa/inet.h>  // inet_ntoa()
#include <netinet/in.h> // inet_ntoa()
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> // inet_ntoa()
#include <unistd.h>

// test begin
void test_formatted_time();
// test end

int main(int argc, char **argv) {
  Option *opt = parse(argc, argv);

  if (opt->test) {
    test_formatted_time();
    return 0;
  }

  server_start(opt);

  return 0;
}

Option *parse(int argvc, char **argv) {
  Option *opts = malloc(sizeof(Option));
  opts->port = 8088;
  opts->test = false;

  return opts;
}

void server_start(Option *opt) {
  FILE *log = fopen("access.log", "a");
  if (log == NULL) {
    perror("fopen");
    exit(1);
  }

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
      worker_start(sock, log, opt);
      delete_socket(sock);
      _exit(0);
    default: // parent
             ;
    }
  }

  delete_socket(sv_sock);
}
