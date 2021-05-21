#include "main.h"
#include "file.h"
#include "net.h"

#include <arpa/inet.h>  // inet_ntoa(3)
#include <netinet/in.h> // inet_ntoa(3)
#include <setjmp.h>     // setjmp(3)
#include <stdlib.h>     // atoi(3)
#include <string.h>     // strcmp(3)
#include <sys/socket.h> // inet_ntoa(3)

static void run_all_test();
static void run_all_test_main();

static Map *new_MimeMap();
static void Option_parse(Option *opts, int argc, char **argv, Exception *ex);
static void print_usage(char *);

jmp_buf g_env;
Map *MimeMap;

int main(int argc, char **argv) {
  Option *opt = calloc(1, sizeof(Option));
  Exception *ex = calloc(1, sizeof(Exception));

  switch (setjmp(g_env)) {
  case 0:
    Option_parse(opt, argc, argv, ex);
    break;
  case EX_ILLEGAL_ARG:
    fprintf(stderr, "%s\n", ex->msg);
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
  // - Map       MimeMap
  // - Option    opt
  // - Exception ex

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

static void Option_parse(Option *opts, int argc, char **argv, Exception *ex) {
  Args *args = new_Args(argc, argv);
  //  Option *opts = calloc(1, sizeof(Option));

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
        ex->msg = "option require an argument -- 'r'";
        delete_Args(args);
        longjmp(g_env, EX_ILLEGAL_ARG);
      }
      opts->document_root = strdup(Args_next(args));
      continue;
    }
    if (strcmp(arg, "-l") == 0) {
      if (!Args_hasNext(args)) {
        ex->msg = "option require an argument -- 'l'";
        delete_Args(args);
        longjmp(g_env, EX_ILLEGAL_ARG);
      }
      opts->access_log = strdup(Args_next(args));
      continue;
    }

    //
    // arguments ...
    //
    if (opts->port < 0) {
      if (atoi(arg) < 0) {
        ex->msg = "PORT must be non-negative";
        delete_Args(args);
        longjmp(g_env, EX_ILLEGAL_ARG);
      }
      opts->port = atoi(arg);
      continue;
    }

    ex->msg = "too many arguments";
    delete_Args(args);
    longjmp(g_env, EX_ILLEGAL_ARG);
  }

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

  delete_Args(args);
}

static void print_usage(char *prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [-r DOCUMENT_ROOT] [-l ACCESS_LOG] [PORT]\n", prog_name);
  fprintf(stderr, "%s -test\n", prog_name);
}

static void test_Option_parse() {
  Option *opt;
  Exception *ex = calloc(1, sizeof(Exception));

  //
  // normal cases...
  //

  switch (setjmp(g_env)) {
  case 0:
    break;
  default:
    error("%s:%d: unexpected exception occurred", __FILE__, __LINE__);
  }

  char *arg_min[] = {"./httpd"};
  opt = calloc(1, sizeof(Option));
  Option_parse(opt, 1, arg_min, ex);
  expect(__LINE__, ex->ty, E_Okay);
  expect(__LINE__, opt->port, 8088);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "www");
  expect_str(__LINE__, opt->access_log, "access.log");
  free(opt);

  char *arg_full[] = {"./httpd", "-r", "root", "-l", "Access.log", "80"};
  opt = calloc(1, sizeof(Option));
  Option_parse(opt, 6, arg_full, ex);
  expect(__LINE__, 80, opt->port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "root");
  expect_str(__LINE__, opt->access_log, "Access.log");
  free(opt);

  char *test_opt[] = {"./httpd", "-test"};
  opt = calloc(1, sizeof(Option));
  Option_parse(opt, 2, test_opt, ex);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_bool(__LINE__, true, opt->test);
  free(opt);

  char *arg_port[] = {"./httpd", "80"};
  opt = calloc(1, sizeof(Option));
  Option_parse(opt, 2, arg_port, ex);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect(__LINE__, 80, opt->port);
  free(opt);

  //
  // abnormal cases...
  //

  char *arg_omit[] = {"./httpd", "-r"};
  switch (setjmp(g_env)) {
  case 0:
    opt = calloc(1, sizeof(Option));
    Option_parse(opt, 2, arg_omit, ex);
    break;
  case EX_ILLEGAL_ARG:
    expect_str(__LINE__, "option require an argument -- 'r'", ex->msg);
    free(opt);
    break;
  }

  char *arg_neg[] = {"./httpd", "-1"};
  switch (setjmp(g_env)) {
  case 0:
    opt = calloc(1, sizeof(Option));
    Option_parse(opt, 2, arg_neg, ex);
    break;
  case EX_ILLEGAL_ARG:
    expect_str(__LINE__, "PORT must be non-negative", ex->msg);
    free(opt);
    break;
  }

  char *arg_many[] = {"./httpd", "80", "80"};
  switch (setjmp(g_env)) {
  case 0:
    opt = calloc(1, sizeof(Option));
    Option_parse(opt, 3, arg_many, ex);
    break;
  case EX_ILLEGAL_ARG:
    expect_str(__LINE__, "too many arguments", ex->msg);
    free(opt);
    break;
  }
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
