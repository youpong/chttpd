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

static Option *parse_args(Args *);
static void print_usage(char *);

char *ErrorMsg;

int main(int argc, char **argv) {
  Option *opt = parse_args(new_args(argc, argv));
  if (opt == NULL) {
    fprintf(stderr, "%s\n", ErrorMsg);
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

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

    return EXIT_SUCCESS;
  }

  server_start(opt);

  return EXIT_SUCCESS;
}

static Option *parse_args(Args *args) {
  Option *opts = calloc(1, sizeof(Option));
  opts->port = -1; // -1: not setted

  opts->prog_name = strdup(args_next(args));

  while (args_has_next(args)) {
    char *arg = args_next(args);

    if (strcmp(arg, "-test") == 0) {
      opts->test = true;
    } else if (strcmp(arg, "-r") == 0) {
      if (!args_has_next(args)) {
        ErrorMsg = strdup("option require an argument -- 'r'");
        return NULL;
      }
      opts->document_root = strdup(args_next(args));
    } else {
      if (opts->port < 0) {
        if (atoi(arg) < 0) {
          ErrorMsg = strdup("PORT must be non-negative");
          return NULL;
        }
        opts->port = atoi(arg);
      } else {
        ErrorMsg = strdup("too many arguments");
        return NULL;
      }
    }
  }

  // set default values...
  if (opts->port < 0) {
    opts->port = DEFAULT_PORT;
  }
  if (opts->document_root == NULL) {
    opts->document_root = strdup("www");
  }

  return opts;
}

static void print_usage(char *prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [-r DOCUMENT_ROOT] [PORT]\n", prog_name);
  fprintf(stderr, "%s -test\n", prog_name);
}

void test_parse_args() {
  Option *opt;

  //
  // normal cases...
  //

  char *arg_min[] = {"./httpd"};
  opt = parse_args(new_args(1, arg_min));
  expect(__LINE__, opt->port, 8088);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "www");

  char *arg_full[] = {"./httpd", "-r", "root", "80"};
  opt = parse_args(new_args(4, arg_full));
  expect(__LINE__, 80, opt->port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "root");

  char *test_opt[] = {"./httpd", "-test"};
  opt = parse_args(new_args(2, test_opt));
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_bool(__LINE__, true, opt->test);

  char *arg_port[] = {"./httpd", "80"};
  opt = parse_args(new_args(2, arg_port));
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect(__LINE__, 80, opt->port);

  //
  // abnormal cases...
  //

  char *arg_omit[] = {"./httpd", "-r"};
  opt = parse_args(new_args(2, arg_omit));
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "option require an argument -- 'r'", ErrorMsg);

  char *arg_neg[] = {"./httpd", "-1"};
  opt = parse_args(new_args(2, arg_neg));
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "PORT must be non-negative", ErrorMsg);

  char *arg_many[] = {"./httpd", "80", "80"};
  opt = parse_args(new_args(3, arg_many));
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "too many arguments", ErrorMsg);
}
