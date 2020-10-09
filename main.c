#include "main.h"

#include <arpa/inet.h>  // inet_ntoa()
#include <netinet/in.h> // inet_ntoa()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // strcmp()
#include <sys/socket.h> // inet_ntoa()
#include <unistd.h>

// test begin
void test_formatted_time();
// test end

static Option *parse_args(int argc, char **argv);
void print_usage();

char *PROG_NAME;

int main(int argc, char **argv) {
  Option *opt = parse_args(argc, argv);

  if (opt->test) {
    test_formatted_time();
    return 0;
  }

  server_start(opt);

  return 0;
}

static Option *parse_args(int argc, char **argv) {
  Option *opts = malloc(sizeof(Option));
  opts->port = 8088;
  opts->test = false;

  PROG_NAME = strdup(argv[0]);

  if (argc == 2 && strcmp(argv[1], "-test") == 0) {
    opts->test = true;
    return opts;
  }

  // process options
  // ...

  // process args
  if (argc > 2) {
    print_usage();
    exit(1);
  } else if (argc == 2) {
    opts->port = atoi(argv[1]);
  }

  return opts;
}

void print_usage() {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [port]\n", PROG_NAME);
  fprintf(stderr, "%s -test\n", PROG_NAME);
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
