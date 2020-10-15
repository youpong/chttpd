#include "main.h"
#include "net.h"
#include <arpa/inet.h> // inet_ntoa()
#include <dirent.h>    // opendir()
#include <fcntl.h>     // open()
#include <stdlib.h>    // malloc()
#include <string.h>    // strcmp()
#include <sys/stat.h>  // open()
#include <sys/types.h> // opendir()
#include <time.h>      // time()
#include <unistd.h>    // write()

static void worker_start(Socket *sock, FILE *log, Option *opt);
static HttpResponse *create_http_response(HttpRequest *, Option *);
static void write_log(FILE *, Socket *, HttpRequest *, HttpResponse *);

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

static void worker_start(Socket *sock, FILE *log, Option *opt) {

  while (true) {
    HttpRequest *req = http_request_parse(sock->fd, opt->debug);
    HttpResponse *res = create_http_response(req, opt);
    write_http_response(sock->fd, res);
    write_log(log, sock, req, res);

    if (strcmp("close", map_get(req->header_map, "Connection")) == 0)
      break;
  }
}

static void set_file(char *dest, FILE *f);

static HttpResponse *create_http_response(HttpRequest *req, Option *opts) {
  HttpResponse *res = malloc(sizeof(HttpResponse));
  res->header_map = new_map();

  if (strcmp(req->method, "GET") == 0 || strcmp(req->method, "HEAD") == 0) {
    char target_path[256];
    strcpy(target_path, opts->document_root);
    strcat(target_path, req->request_uri);
    FILE *target = fopen(target_path, "r");
    DIR *d = opendir(req->request_uri);
    if (target != NULL && d == NULL) {
      res->status_code = strdup("200");
      res->reason_phrase = strdup("OK");
    } else {
      res->status_code = strdup("404");
      res->reason_phrase = strdup("Not Found");
      return res;
    }

    // Http Version
    res->http_version = strdup(HTTP_VERSION);

    // Server
    map_put(res->header_map, "Server", SERVER_NAME);

    // Content-Type
    map_put(res->header_map, "Content-Type", "text/html");

    // Body
    res->body = malloc(1024 * 64);
    set_file(res->body, target);

    // Content-Length: TODO
    char *buf = malloc(6);
    sprintf(buf, "%ld", strlen(res->body));
    map_put(res->header_map, "Content-Length", buf);
  } else {
    // Not Allowed
    res->status_code = strdup("405");
    res->reason_phrase = strdup("Not Allowed");
  }

  return res;
}

static void set_file(char *dest, FILE *f) {
  int c;
  char *p = dest;

  while ((c = fgetc(f)) != EOF) {
    *p++ = c;
  }
  *p = '\0';
}

static char *default_val(char *val, char *dval);
static char *formatted_time(struct tm *, long);
static bool is_locked();
static int lock();
static void unlock();

static void write_log(FILE *out, Socket *sock, HttpRequest *req,
                      HttpResponse *res) {
  time_t req_time;
  struct tm req_tm;

  time(&req_time);
  localtime_r(&req_time, &req_tm);

  do {
    while (is_locked()) {
      sleep(1);
    }
  } while (lock() == -1);

  // clang-format off
  fprintf(out, "%s - - [%s] \"%s %s %s\" %s %s \"%s\" \"%s\"\n",
      inet_ntoa(sock->addr->sin_addr),
      // date [09/Oct/2020:13:17:45 +0900]
      formatted_time(&req_tm, timezone),
      // request-line
      req->method, req->request_uri, req->http_version,
      res->status_code,
      default_val((char *)map_get(res->header_map, "Content-Length"), "\"-\""),
      default_val((char *)map_get(req->header_map, "Referer"), "-"),
      default_val((char *)map_get(req->header_map, "User-Agent"), "-"));
  // clang-format on
  fflush(out);

  unlock();
}

/**
 * e.g.
 * 09/Oct/2020:17:34:23 +0900
 */
static char *formatted_time(struct tm *t_tm, long timezone) {
  char buf[26 + 1];
  char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  // clang-format off
  sprintf(buf, "%02d/%s/%d:%02d:%02d:%02d %+03d%02d",
          t_tm->tm_mday,
          month[t_tm->tm_mon],
          t_tm->tm_year + 1900,
          t_tm->tm_hour,
          t_tm->tm_min,
          t_tm->tm_sec,	  
          (int)-timezone / (60 * 60),
          abs(timezone) % (60 * 60) / 60);
  // clang-format on

  return strdup(buf);
}

static bool is_locked() {
  FILE *f = fopen("/var/lock/dali.pid", "r");
  return f != NULL;
}

static int lock() {
  int fd = open("/var/lock/dali.pid", O_CREAT | O_EXCL);
  return fd;
}

static void unlock() {
  unlink("/var/lock/dali.pid");
}

static char *default_val(char *val, char *dval) {
  return val ? val : dval;
}

void test_formatted_time() {
  time_t t = 0; // Epoch 1970.01.01 00:00:00 +0000(UTC)
  struct tm t_tm;

  gmtime_r(&t, &t_tm);

  expect_str(__LINE__, "01/Jan/1970:00:00:00 +0000", formatted_time(&t_tm, 0));
  expect_str(__LINE__, "01/Jan/1970:00:00:00 +0000", formatted_time(&t_tm, 59));

  expect_str(__LINE__, "01/Jan/1970:00:00:00 -0930",
             formatted_time(&t_tm, 9 * 60 * 60 + 30 * 60));
  expect_str(__LINE__, "01/Jan/1970:00:00:00 +0930",
             formatted_time(&t_tm, -(9 * 60 * 60 + 30 * 60)));
  expect_str(__LINE__, "01/Jan/1970:00:00:00 +0829",
             formatted_time(&t_tm, -(9 * 60 * 60 - 30 * 60) + 1));
  expect_str(__LINE__, "01/Jan/1970:00:00:00 +0830",
             formatted_time(&t_tm, -(9 * 60 * 60 - 30 * 60)));
  expect_str(__LINE__, "01/Jan/1970:00:00:00 +0830",
             formatted_time(&t_tm, -(9 * 60 * 60 - 30 * 60) - 1));

  // t = 2,147,483,647;
  t = 1602589880;
  gmtime_r(&t, &t_tm);
  expect_str(__LINE__, "13/Oct/2020:11:51:20 +0900",
             formatted_time(&t_tm, -(9 * 60 * 60)));
}

void test_create_http_response() {
  HttpResponse *res;
  HttpRequest *req = malloc(sizeof(HttpResponse));
  req->header_map = new_map();
  Option *opt = malloc(sizeof(Option));
  opt->document_root = strdup("www");

  req->method = strdup("FOO");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "405", res->status_code);

  req->method = strdup("GET");
  req->request_uri = strdup("/not_exist");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "404", res->status_code);

  req->method = strdup("GET");
  req->request_uri = strdup("/hello.html");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "200", res->status_code);
}

void test_set_file() {
  char buf[2048];
  FILE *f = fopen("LICENSE", "r");
  set_file(buf, f);
  fclose(f);

  expect(__LINE__, 'M', buf[0]);
  expect(__LINE__, 1064, strlen(buf));
}

void test_write_log() {
  FILE *log = fopen("log", "w");
  Socket *sock = create_server_socket(8081);
  HttpRequest *req = malloc(sizeof(HttpRequest));
  req->header_map = new_map();
  req->method = strdup("GET");
  req->request_uri = strdup("/hello.html");
  req->http_version = strdup("HTTP/1.1");
  map_put(req->header_map, "Referer", "http://localhost:8080/hello2.html");
  map_put(req->header_map, "User-Agent", "Dali/0.1");
  HttpResponse *res = malloc(sizeof(HttpResponse));
  res->header_map = new_map();
  res->status_code = strdup("200");
  map_put(res->header_map, "Content-Length", "199");

  write_log(log, sock, req, res);
}
