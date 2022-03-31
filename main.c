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
    if (opt->help) {
        print_usage(opt->prog_name);
        return EXIT_SUCCESS;
    }
    if (opt->version) {
        fprintf(stderr, "%s %s\n", opt->prog_name, VERSION);
        return EXIT_SUCCESS;
    }

    MimeMap = new_MimeMap();
    server_start(opt);

    return EXIT_SUCCESS;
}

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

    opts->prog_name = ArgsIter_next(iter);

    //
    // set default values...
    //
    opts->port = DEFAULT_PORT;
    opts->document_root = "www";
    opts->access_log = "access.log";

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
            if (strcmp(arg, "-h") == 0) {
                opts->help = true;
                continue;
            }
            if (strcmp(arg, "-v") == 0) {
                opts->version = true;
                continue;
            }
            if (strcmp(arg, "-r") == 0) {
                if (!ArgsIter_hasNext(iter)) {
                    ex->ty = O_IllegalArgument;
                    ex->msg = "option require an argument -- 'r'";
                    break;
                }
                opts->document_root = ArgsIter_next(iter);
                continue;
            }
            if (strcmp(arg, "-l") == 0) {
                if (!ArgsIter_hasNext(iter)) {
                    ex->ty = O_IllegalArgument;
                    ex->msg = "option require an argument -- 'l'";
                    break;
                }
                opts->access_log = ArgsIter_next(iter);
                continue;
            }
            if (strcmp(arg, "-p") == 0) {
                if (!ArgsIter_hasNext(iter)) {
                    ex->ty = O_IllegalArgument;
                    ex->msg = "option require an argument -- 'p'";
                    break;
                }
                opts->port = atoi(ArgsIter_next(iter));
                continue;
            }
            // else ...
            ex->ty = O_IllegalArgument;
            ex->msg = "unknown option";
            break;
        }
        // else ...
        ex->ty = O_IllegalArgument;
        ex->msg = "unknown argument";
        break;
    }

    return opts;
}

static void print_usage(char *prog_name) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [-r DOCUMENT_ROOT] [-l ACCESS_LOG] [-p PORT]\n",
            prog_name);
    fprintf(stderr, "%s -h\n", prog_name);
    fprintf(stderr, "%s -v\n", prog_name);
}

/**
 * normal cases...
 */
static void test_Option_parse_normal() {
    Option *opt;
    Exception *ex = calloc(1, sizeof(Exception));

    char *arg_min[] = {"./httpd"};
    opt = Option_parse(1, arg_min, ex);
    expect(__LINE__, ex->ty, E_Okay);
    expect(__LINE__, opt->port, 8088);
    expect_str(__LINE__, opt->prog_name, "./httpd");
    expect_str(__LINE__, opt->document_root, "www");
    expect_str(__LINE__, opt->access_log, "access.log");

    char *arg_full[] = {"./HTTPD", "-r", "WWW", "-l", "ACCESS.LOG", "-p", "80"};
    opt = Option_parse(7, arg_full, ex);
    expect(__LINE__, ex->ty, E_Okay);
    expect(__LINE__, 80, opt->port);
    expect_str(__LINE__, opt->prog_name, "./HTTPD");
    expect_str(__LINE__, opt->document_root, "WWW");
    expect_str(__LINE__, opt->access_log, "ACCESS.LOG");

    char *arg_help[] = {"./httpd", "-h"};
    opt = Option_parse(2, arg_help, ex);
    expect(__LINE__, ex->ty, E_Okay);
    expect_bool(__LINE__, true, opt->help);

    char *arg_ver[] = {"./httpd", "-v"};
    opt = Option_parse(2, arg_ver, ex);
    expect(__LINE__, ex->ty, E_Okay);
    expect_bool(__LINE__, true, opt->version);

    char *arg_test[] = {"./httpd", "-test"};
    opt = Option_parse(2, arg_test, ex);
    expect(__LINE__, ex->ty, E_Okay);
    expect_bool(__LINE__, true, opt->test);
}

/**
 * abnormal cases...
 */
static void test_Option_parse_abnormal() {
    Option *opt;
    Exception *ex = calloc(1, sizeof(Exception));

    ex->ty = E_Okay;
    char *arg_r[] = {"./httpd", "-r"};
    opt = Option_parse(2, arg_r, ex);
    expect(__LINE__, ex->ty, O_IllegalArgument);
    expect_str(__LINE__, "option require an argument -- 'r'", ex->msg);

    ex->ty = E_Okay;
    char *arg_l[] = {"./httpd", "-l"};
    opt = Option_parse(2, arg_l, ex);
    expect(__LINE__, ex->ty, O_IllegalArgument);
    expect_str(__LINE__, "option require an argument -- 'l'", ex->msg);

    ex->ty = E_Okay;
    char *arg_p[] = {"./httpd", "-p"};
    opt = Option_parse(2, arg_p, ex);
    expect(__LINE__, ex->ty, O_IllegalArgument);
    expect_str(__LINE__, "option require an argument -- 'p'", ex->msg);

    ex->ty = E_Okay;
    char *arg_unknown_opt[] = {"./httpd", "-1"};
    opt = Option_parse(2, arg_unknown_opt, ex);
    expect_str(__LINE__, "unknown option", ex->msg);

    ex->ty = E_Okay;
    char *arg_unknown_arg[] = {"./httpd", "a"};
    opt = Option_parse(2, arg_unknown_arg, ex);
    expect_str(__LINE__, "unknown argument", ex->msg);
}

static void run_all_test_main() {
    test_Option_parse_normal();
    test_Option_parse_abnormal();
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
