#include "main.h"
#include "file.h"
#include "net.h"

#include <arpa/inet.h>  // inet_ntoa(3)
#include <netinet/in.h> // inet_ntoa(3)
#include <stdlib.h>     // atoi(3)
#include <string.h>     // strcmp(3)
#include <sys/socket.h> // inet_ntoa(3)

static void run_all_test();
static void run_all_test_main();
void run_all_test_server();

static Map *new_MimeMap();
static Option *Option_parse(int argc, char **argv);
static void print_usage(char *);

Map *MimeMap;
char *ErrorMsg;

int main(int argc, char **argv) {

  Option *opt = Option_parse(argc, argv);
  if (opt == NULL) {
    fprintf(stderr, "%s\n", ErrorMsg);
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (opt->test) {
    run_all_test();
    return EXIT_SUCCESS;
  }

  MimeMap = new_MimeMap();
  server_start(opt);

  // System will delete bellow objects soon.
  // - MimeMap
  // - opt

  return EXIT_SUCCESS;
}

// MimeMap: don't use delete_Map()
static Map *new_MimeMap() {
  Map *map = new_Map();

  // clang-format off
  Map_put(map, "css" , "text/css" );
  Map_put(map, "gif" , "image/gif");
  Map_put(map, "html", "text/html");
  Map_put(map, "png" , "image/png");
  // clang-format on

  return map;
}

static Option *Option_parse(int argc, char **argv) {
  Args *args = new_Args(argc, argv);
  Option *opts = calloc(1, sizeof(Option));

  opts->port = -1; // -1: not setted

  opts->prog_name = strdup(Args_next(args));

  while (Args_hasNext(args)) {
    char *arg = Args_next(args);

    //
    // options ...
    //
    if (strcmp(arg, "-test") == 0) {
      opts->test = true;
      continue;
    }
    if (strcmp(arg, "-r") == 0) {
      if (!Args_hasNext(args)) {
        ErrorMsg = "option require an argument -- 'r'";
        goto IllegalArgument;
      }
      opts->document_root = strdup(Args_next(args));
      continue;
    }
    if (strcmp(arg, "-l") == 0) {
      if (!Args_hasNext(args)) {
        ErrorMsg = "option require an argument -- 'l'";
        goto IllegalArgument;
      }
      opts->access_log = strdup(Args_next(args));
      continue;
    }

    //
    // arguments ...
    //
    if (opts->port < 0) {
      if (atoi(arg) < 0) {
        ErrorMsg = "PORT must be non-negative";
        goto IllegalArgument;
      }
      opts->port = atoi(arg);
      continue;
    }

    ErrorMsg = "too many arguments";
    goto IllegalArgument;
  }
  delete_Args(args);

  //
  // set default values...
  //
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

  //
  // catch parse error
  //
IllegalArgument:
  delete_Args(args);
  free(opts);
  return NULL;
}

static void print_usage(char *prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [-r DOCUMENT_ROOT] [-l ACCESS_LOG] [PORT]\n", prog_name);
  fprintf(stderr, "%s -test\n", prog_name);
}

static void test_Option_parse() {
  Option *opt;

  //
  // normal cases...
  //

  char *arg_min[] = {"./httpd"};
  opt = Option_parse(1, arg_min);
  expect(__LINE__, opt->port, 8088);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "www");
  expect_str(__LINE__, opt->access_log, "access.log");
  free(opt);

  char *arg_full[] = {"./httpd", "-r", "root", "-l", "Access.log", "80"};
  opt = Option_parse(6, arg_full);
  expect(__LINE__, 80, opt->port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "root");
  expect_str(__LINE__, opt->access_log, "Access.log");
  free(opt);

  char *test_opt[] = {"./httpd", "-test"};
  opt = Option_parse(2, test_opt);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_bool(__LINE__, true, opt->test);
  free(opt);

  char *arg_port[] = {"./httpd", "80"};
  opt = Option_parse(2, arg_port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect(__LINE__, 80, opt->port);
  free(opt);

  //
  // abnormal cases...
  //

  char *arg_omit[] = {"./httpd", "-r"};
  opt = Option_parse(2, arg_omit);
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "option require an argument -- 'r'", ErrorMsg);
  free(opt);

  char *arg_neg[] = {"./httpd", "-1"};
  opt = Option_parse(2, arg_neg);
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "PORT must be non-negative", ErrorMsg);
  free(opt);

  char *arg_many[] = {"./httpd", "80", "80"};
  opt = Option_parse(3, arg_many);
  expect_ptr(__LINE__, NULL, opt);
  expect_str(__LINE__, "too many arguments", ErrorMsg);
  free(opt);
}

static void run_all_test_main() {
  test_Option_parse();
}

static void run_all_test() {
  MimeMap = new_MimeMap();

  run_all_test_util();
  run_all_test_main();
  run_all_test_file();
  run_all_test_net();
  run_all_test_server();

  printf("========================\n");
  printf(" All unit tests passed.\n");
  printf("========================\n");
}
