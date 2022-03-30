/**
 * @file main.c
 */
#include "main.h"
#include "file.h"
#include "net.h"

#include <stdlib.h> // atoi(3)
#include <string.h> // strcmp(3)

static void run_all_test();
static void run_all_test_main();

static Map *new_MimeMap();
static Option *Option_parse(int argc, char **argv, Exception *ex);
static void print_usage(char *);

/** a Mime map*/
Map *MimeMap;

/**
 *
 */
int main(int argc, char **argv) {
  Exception *ex = calloc(1, sizeof(Exception));

  Option *opt = Option_parse(argc, argv, ex);
  if (ex->ty != E_Okay) {
    fprintf(stderr, "%s\n", ex->msg);
    print_usage(opt->prog_name);
    return EXIT_FAILURE;
  }

  if (opt->test) {
    run_all_test();
    return EXIT_SUCCESS;
  }

  if (opt->version) {
    fprintf(stderr, "%s %s\n", opt->prog_name, VERSION);
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
  Map_put(map, "wmv" , "video/x-ms-wmv");
  // clang-format on

  return map;
}

static Option *Option_parse(int argc, char **argv, Exception *ex) {
  ArgsIter *iter = new_ArgsIter(argc, argv);
  Option *opts = calloc(1, sizeof(Option));

  opts->port = -1; // -1: not setted

  opts->prog_name = strdup(ArgsIter_next(iter));

  while (ArgsIter_hasNext(iter)) {
    char *arg = ArgsIter_next(iter);

    if (arg[0] == '-') {
      //
      // options ...
      //
      if (strcmp(arg, "-test") == 0) {
        opts->test = true;
        continue;
      }
      if (strcmp(arg, "-v") == 0) {
        opts->version = true;
        continue;
      }
      if (strcmp(arg, "-r") == 0) {
        if (!ArgsIter_hasNext(iter)) {
          ex->msg = "option require an argument -- 'r'";
          goto IllegalArgument;
        }
        opts->document_root = strdup(ArgsIter_next(iter));
        continue;
      }
      if (strcmp(arg, "-l") == 0) {
        if (!ArgsIter_hasNext(iter)) {
          ex->msg = "option require an argument -- 'l'";
          goto IllegalArgument;
        }
        opts->access_log = strdup(ArgsIter_next(iter));
        continue;
      }
      if (strcmp(arg, "-p") == 0) {
        if (!ArgsIter_hasNext(iter)) {
          ex->msg = "option require an argument -- 'p'";
          goto IllegalArgument;
        }
        opts->port = atoi(ArgsIter_next(iter));
        continue;
      }

      ex->msg = "unknown option";
      goto IllegalArgument;
    }

    ex->msg = "unknown argument";
    goto IllegalArgument;
  }
  delete_ArgsIter(iter);

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
  ex->ty = O_IllegalArgument;
  delete_ArgsIter(iter);
  return opts;
}

static void print_usage(char *prog_name) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s [-r DOCUMENT_ROOT] [-l ACCESS_LOG] [-p PORT]\n", prog_name);
  fprintf(stderr, "%s -v\n", prog_name);
  fprintf(stderr, "%s -test\n", prog_name);
}

static void test_Option_parse() {
  Option *opt;
  Exception *ex = calloc(1, sizeof(Exception));

  //
  // normal cases...
  //

  char *arg_min[] = {"./httpd"};
  opt = Option_parse(1, arg_min, ex);
  expect(__LINE__, ex->ty, E_Okay);
  expect(__LINE__, opt->port, 8088);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "www");
  expect_str(__LINE__, opt->access_log, "access.log");
  free(opt);

  char *arg_full[] = {"./httpd", "-r", "root", "-l", "Access.log", "-p", "80"};
  opt = Option_parse(8, arg_full, ex);
  expect(__LINE__, 80, opt->port);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_str(__LINE__, opt->document_root, "root");
  expect_str(__LINE__, opt->access_log, "Access.log");
  free(opt);

  char *test_opt[] = {"./httpd", "-test"};
  opt = Option_parse(2, test_opt, ex);
  expect_str(__LINE__, opt->prog_name, "./httpd");
  expect_bool(__LINE__, true, opt->test);
  free(opt);

  //
  // abnormal cases...
  //

  char *arg_omit[] = {"./httpd", "-r"};
  opt = Option_parse(2, arg_omit, ex);
  expect(__LINE__, ex->ty, O_IllegalArgument);
  expect_str(__LINE__, "option require an argument -- 'r'", ex->msg);
  free(opt);

  char *arg_unknown[] = {"./httpd", "-1"};
  opt = Option_parse(2, arg_unknown, ex);
  expect_str(__LINE__, "unknown option", ex->msg);
  free(opt);

  char *arg_nan[] = {"./httpd", "a"};
  opt = Option_parse(2, arg_nan, ex);
  expect_str(__LINE__, "unknown argument", ex->msg);
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

  printf("==============================\n");
  printf(" All unit tests passed.\n");
  printf("==============================\n");
}
