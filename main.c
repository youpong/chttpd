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
void test_create_http_response();
void test_http_request_parse();
// test end

static Option *parse_args(int argc, char **argv);
static void print_usage();

char *PROG_NAME;

int main(int argc, char **argv) {
  Option *opt = parse_args(argc, argv);

  if (opt->test) {
    test_formatted_time();
    test_create_http_response();
    test_http_request_parse();
    return 0;
  }

  server_start(opt);

  return 0;
}

static Option *parse_args(int argc, char **argv) {
  Option *opts = malloc(sizeof(Option));
  opts->port = 8088;
  opts->test = false;
  opts->debug = false;
  opts->document_root = strdup("www");

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

static void print_usage() {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [port]\n", PROG_NAME);
  fprintf(stderr, "%s -test\n", PROG_NAME);
}
