#include "file.h"
#include "main.h"
#include "net.h"
#include "util.h"

#include <arpa/inet.h> // inet_ntoa(3)
#include <fcntl.h>     // open(2)
#include <stdlib.h>    // malloc(3)
#include <string.h>    // strcmp(3), strrchr(3)
#include <sys/stat.h>  // open(2)
#include <time.h>      // time(2)
#include <unistd.h>    // unlink(2)

static void handle_connection(Socket *sock, FILE *log, Option *opt);
static HttpMessage *create_http_response(HttpMessage *, Option *);
static void write_log(FILE *, Socket *, time_t *, HttpMessage *, HttpMessage *);

void server_start(Option *opt) {
  FILE *log = fopen(opt->access_log, "a");
  if (log == NULL) {
    perror("fopen");
    exit(1);
  }

  Socket *sv_sock = create_server_socket(opt->port);
  printf("listen: %s:%d\n", inet_ntoa(sv_sock->addr->sin_addr),
         ntohs(sv_sock->addr->sin_port));

  for (int i = 0; i < LISTEN_QUEUE; i++) {
    pid_t pid = fork();
    switch (pid) {
    case -1: // error
      perror("fork");
      exit(1);
    case 0: // child
      while (true) {
        Socket *sock = server_accept(sv_sock);
        printf("open pid: %d, address: %s, port: %d\n", getpid(),
               inet_ntoa(sock->addr->sin_addr), ntohs(sock->addr->sin_port));
        handle_connection(sock, log, opt);
        delete_socket(sock);
      }
      break;
    default: // parent
             /* no-op */;
    }
  }

  pause();

  fclose(log);
  delete_socket(sv_sock);
}

static void handle_connection(Socket *sock, FILE *log, Option *opt) {
  time_t req_time;
  bool conn_keep_alive = true;

  while (conn_keep_alive) {
    time(&req_time);

    HttpMessage *req = http_message_parse(sock->ips, HM_REQ, opt->debug);
    if (req == NULL)
      break;

    HttpMessage *res = create_http_response(req, opt);

    write_http_message(sock->ops, res);
    write_log(log, sock, &req_time, req, res);

    char *conn = map_get(req->header_map, "Connection");
    if (conn != NULL && strcmp(conn, "close") == 0)
      conn_keep_alive = false;

    delete_HttpMessage(req);
    delete_HttpMessage(res);
  }
}

static void header_put(HttpMessage *msg, char *key, char *value);
static int file_read(File *file, char *dest); // extern ?
static char *get_mime_type(char *fname);

static HttpMessage *create_http_response(HttpMessage *req, Option *opts) {
  HttpMessage *res = new_HttpMessage(HM_RES);

  if (strcmp(req->method, "GET") == 0 || strcmp(req->method, "HEAD") == 0) {
    File *file;

    // HTTP-Version
    res->http_version = strdup(HTTP_VERSION);

    // Status-Code, Reason-Phrase
    file = new_file2(opts->document_root, req->filename);
    if (file != NULL && file->ty == F_FILE) {
      res->status_code = strdup("200");
      res->reason_phrase = strdup("OK");
    } else {
      file = new_file2(opts->document_root, "/error.html");
      res->status_code = strdup("404");
      res->reason_phrase = strdup("Not Found");
    }

    // Server
    header_put(res, "Server", SERVER_NAME);

    // Content-Type
    header_put(res, "Content-Type", get_mime_type(file->path));

    // Content-Length
    char buf[20 + 1]; // log10(ULONG_MAX) < 20
    sprintf(buf, "%d", file->len);
    header_put(res, "Content-Length", buf);

    // Body (omit if POST method)
    if (strcmp(req->method, "GET") == 0) {
      res->body = malloc(file->len + 1);
      res->body_len = file_read(file, res->body);
    }

    delete_file(file);
  } else {
    // Not Allowed Request method
    res->status_code = strdup("405");
    res->reason_phrase = strdup("Not Allowed");
  }

  return res;
}

static void header_put(HttpMessage *msg, char *key, char *value) {
  map_put(msg->header_map, strdup(key), strdup(value));
}

static char *get_mime_type(char *path) {
  char *ext = extension(path);
  if (ext == NULL)
    return "text/plain";

  char *mime = map_get(Mime_map, ext);
  free(ext);
  
  if (mime == NULL)
    return "text/plain";

  return mime;
}

static int file_read(File *file, char *dest) {
  int fd = open(file->path, O_RDONLY);
  int len = read(fd, dest, file->len);
  close(fd);

  return len;
}

static char *default_val(char *val, char *dval);
static char *formatted_time(struct tm *, long);
static bool is_locked();
static int lock();
static void unlock();

static void write_log(FILE *out, Socket *sock, time_t *req_time,
                      HttpMessage *req, HttpMessage *res) {
  struct tm req_tm;
  localtime_r(req_time, &req_tm);

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
  bool ret;
  FILE *f;

  if ((f = fopen("/var/lock/dali.pid", "r")) != NULL) {
    fclose(f);
    ret = true;
  } else
    ret = false;

  return ret;
}

static int lock() {
  int fd = open("/var/lock/dali.pid", O_CREAT | O_EXCL);
  close(fd);
  return fd;
}

static void unlock() {
  unlink("/var/lock/dali.pid");
}

static char *default_val(char *val, char *dval) {
  return val ? val : dval;
}

static void test_formatted_time() {
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

static void test_create_http_response() {
  HttpMessage *res;
  HttpMessage *req = new_HttpMessage(HM_REQ);
  Option *opt = malloc(sizeof(Option));
  opt->document_root = strdup("www");

  // Not Allowed Request method
  req->method = strdup("FOO");
  res = create_http_response(req, opt);
  expect(__LINE__, HM_RES, res->_ty);
  expect_str(__LINE__, "405", res->status_code);

  // GET not exist filename
  req->method = strdup("GET");
  req->request_uri = strdup("/not_exist");
  req->filename = strdup("/not_exist");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "404", res->status_code);
  expect_bool(__LINE__, true,
              res->body_len ==
                  atoi(map_get(res->header_map, "Content-Length")));

  // HEAD not exist filename
  req->method = strdup("HEAD");
  req->request_uri = strdup("/not_exist");
  req->filename = strdup("/not_exist");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "404", res->status_code);
  expect_ptr(__LINE__, NULL, res->body);

  // GET
  req->method = strdup("GET");
  req->request_uri = strdup("/hello.html");
  req->filename = strdup("/hello.html");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "200", res->status_code);
  expect_bool(__LINE__, true,
              res->body_len ==
                  atoi(map_get(res->header_map, "Content-Length")));

  // HEAD
  req->method = strdup("HEAD");
  req->request_uri = strdup("/hello.html");
  req->filename = strdup("/hello.html");
  res = create_http_response(req, opt);
  expect_str(__LINE__, "200", res->status_code);
  expect_ptr(__LINE__, NULL, res->body);
}

static void test_file_read() {
  File *file = new_file("LICENSE");

  char *buf = malloc(file->len);
  int len = file_read(file, buf);

  expect(__LINE__, 'M', buf[0]);
  expect(__LINE__, 1064, len);
}

static void test_write_log() {
  //
  // write log
  //
  Socket *sock = create_server_socket(8081);

  HttpMessage *req = new_HttpMessage(HM_REQ);
  req->method = strdup("GET");
  req->request_uri = strdup("/hello.html");
  req->http_version = strdup("HTTP/1.1");
  header_put(req, "Referer", "http://localhost:8080/hello2.html");
  header_put(req, "User-Agent", "Dali/0.1");

  HttpMessage *res = new_HttpMessage(HM_RES);
  res->status_code = strdup("200");
  header_put(res, "Content-Length", "199");

  time_t req_time = 1602737916;
  FILE *log = fopen("log", "w");
  write_log(log, sock, &req_time, req, res);
  fclose(log);
  delete_HttpMessage(req);
  delete_HttpMessage(res);

  //
  // read log
  //

  File *f = new_file("log");
  char *buf = malloc(f->len);
  file_read(f, buf);
  delete_file(f);
  unlink("log");

  //
  // check
  //

  // clang-format off
  expect_str(__LINE__,
    "0.0.0.0 "                      // client ip addr
    "- "                            // client identity(identd)
    "- "                            // user id
    "[15/Oct/2020:13:58:36 +0900] " // request accept time
    "\"GET /hello.html HTTP/1.1\" " // request-line
    "200 "                          // Status-Code
    "199 "                          // content length
    "\"http://localhost:8080/hello2.html\" " // Referer
    "\"Dali/0.1\"\n",               // User-Agent
    buf);
  // clang-format on
}

static void test_get_mime_type() {
  expect_str(__LINE__, "text/html", get_mime_type("index.html"));
  expect_str(__LINE__, "text/plain", get_mime_type("extless"));
  expect_str(__LINE__, "text/plain", get_mime_type("a.unknownext"));
  expect_str(__LINE__, "text/plain", get_mime_type(".dotfile"));
}

void run_all_test_server() {
  test_get_mime_type();
  test_formatted_time();
  test_create_http_response();
  test_file_read();
  test_write_log();
}
