/** @file */
#pragma once

#include "util.h"

// clang-format off
#define VERSION      "0.1.0"
#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME  "Dali"
#define DEFAULT_PORT 8088
#define MAX_SERVERS  20
// clang-format on

typedef struct {
    char *prog_name;
    bool debug;
    bool help;
    bool test;
    bool version;
    char *document_root;
    char *access_log;
    int port;
} Option;

void server_start(Option *);
void run_all_test_server();

/** a Mime map*/
extern Map *MimeMap;
