#include "main.h"

#include <arpa/inet.h>  // inet_ntoa()
#include <netinet/in.h> // inet_ntoa()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // strcmp()
#include <sys/socket.h> // inet_ntoa()
#include <unistd.h>

// test begin
void test_parse_args();
void test_formatted_time();
void test_create_http_response();
void test_write_http_message();
void test_http_message_parse();
void test_set_file();
void test_write_log();
// test end

static Option *parse_args(int argc, char **argv);
static void print_usage();

int main(int argc, char **argv) {
  Option *opt = parse_args(argc, argv);

  if (opt->test) {
    test_parse_args();
    test_formatted_time();
    test_create_http_response();
    test_http_message_parse();
    test_write_http_message();
    test_set_file();
    test_write_log();
    run_utiltest();

    printf("========================\n");
    printf(" All unit tests passed.\n");
    printf("========================\n");

    return 0;
  }

  server_start(opt);

  return 0;
}

static Option *parse_args(int argc, char **argv) {
  Args *args = new_args(argc, argv);
  Option *opts = calloc(1, sizeof(Option));

  opts->prog_name = strdup(*argv);

  argc--;
  argv++;
  while (argc > 0) {
    // char *arg = *argv;
    if (strcmp(*argv, "-test") == 0) {
      opts->test = true;
    } else if (strcmp(*argv, "-r") == 0) {
      // TODO: no arg exception
      argc--;
      argv++;
      opts->document_root = strdup(*argv);
    } else {
      if (opts->port == 0)
        opts->port = atoi(*argv);
      else
        print_usage();
    }

    argc--;
    argv++;
  }

  // set default values...
  if (opts->port == 0) {
    opts->port = DEFAULT_PORT;
  }
  if (opts->document_root == NULL) {
    opts->document_root = strdup("www");
  }

  return opts;
}

static void print_usage(Option *opts) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [-d DOCUMENT_ROOT] [PORT]\n", opts->prog_name);
  fprintf(stderr, "%s -test\n", opts->prog_name);
}

void test_parse_args() {
  Option *opt;
  char *minimum[] = {"./httpd"};

  opt = parse_args(1, minimum);
  expect(__LINE__, opt->port, 8088);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "www");

  char *test_opt[] = {"./httpd", "-test"};
  opt = parse_args(2, test_opt);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_bool(__LINE__, true, opt->test);

  char *arg_port[] = {"./httpd", "80"};
  opt = parse_args(2, arg_port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect(__LINE__, 80, opt->port);
}
