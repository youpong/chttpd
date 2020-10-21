#include "main.h"
#include "util.h"

#include <arpa/inet.h>  // inet_ntoa()
#include <netinet/in.h> // inet_ntoa()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     // strcmp()
#include <sys/socket.h> // inet_ntoa()
#include <unistd.h>

Map *Mime_map;

void run_all_test_main();
void run_all_test_server();

static void init_mime_map();
static Option *parse_args(int, char **);
static void print_usage(char *);

char *ErrorMsg;

int main(int argc, char **argv) {

  Option *opt = parse_args(argc, argv);
  if (opt == NULL) {
    fprintf(stderr, "%s\n", ErrorMsg);
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  init_mime_map();

  if (opt->test) {
    run_all_test_main();
    run_all_test_server();
    run_all_test_net();
    run_all_test_util();

    printf("========================\n");
    printf(" All unit tests passed.\n");
    printf("========================\n");

    return EXIT_SUCCESS;
  }

  server_start(opt);

  return EXIT_SUCCESS;
}

static void init_mime_map() {
  Mime_map = new_map();

  // clang-format off
  map_put(Mime_map, "css" , "text/css" );
  map_put(Mime_map, "gif" , "image/gif");
  map_put(Mime_map, "html", "text/html");
  map_put(Mime_map, "png" , "image/png");
  // clang-format on  
}

static Option *parse_args(int argc, char **argv) {
  Args *args = new_args(argc, argv);
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
    } else if (strcmp(arg, "-l") == 0) {
      if (!args_has_next(args)) {
        ErrorMsg = strdup("option require an argument -- 'l'");
        return NULL;
      }
      opts->access_log = strdup(args_next(args));
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
  if (opts->access_log == NULL) {
    opts->access_log = strdup("access.log");
  }

  return opts;
}

static void print_usage(char *prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [-r DOCUMENT_ROOT] [-l ACCESS_LOG] [PORT]\n", prog_name);
  fprintf(stderr, "%s -test\n", prog_name);
}

static void test_parse_args() {
  Option *opt;

  //
  // normal cases...
  //

  char *arg_min[] = {"./httpd"};
  opt = parse_args(1, arg_min);
  expect(__LINE__, opt->port, 8088);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "www");
  expect_str(__LINE__, opt->access_log, "access.log");

  char *arg_full[] = {"./httpd", "-r", "root", "-l", "Access.log", "80"};
  opt = parse_args(6, arg_full);
  expect(__LINE__, 80, opt->port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "root");
  expect_str(__LINE__, opt->access_log, "Access.log");

  char *test_opt[] = {"./httpd", "-test"};
  opt = parse_args(2, test_opt);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_bool(__LINE__, true, opt->test);

  char *arg_port[] = {"./httpd", "80"};
  opt = parse_args(2, arg_port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect(__LINE__, 80, opt->port);

  //
  // abnormal cases...
  //

  char *arg_omit[] = {"./httpd", "-r"};
  opt = parse_args(2, arg_omit);
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "option require an argument -- 'r'", ErrorMsg);

  char *arg_neg[] = {"./httpd", "-1"};
  opt = parse_args(2, arg_neg);
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "PORT must be non-negative", ErrorMsg);

  char *arg_many[] = {"./httpd", "80", "80"};
  opt = parse_args(3, arg_many);
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "too many arguments", ErrorMsg);
}

void run_all_test_main() {
  test_parse_args();
}
