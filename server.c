#include "main.h"
#include "net.h"
#include "util.h"
#include <arpa/inet.h> // inet_ntoa()
#include <dirent.h>    // opendir()
#include <fcntl.h>     // open()
#include <stdlib.h>    // malloc()
#include <string.h>    // strcmp(), strrchr()
#include <sys/stat.h>  // open()
#include <sys/types.h> // opendir()
#include <time.h>      // time()
#include <unistd.h>    // write()

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

static int set_file(char *dest, FILE *f);
static char *get_mime_type(char *fname);

static HttpMessage *create_http_response(HttpMessage *req, Option *opts) {
  HttpMessage *res = new_HttpMessage(HM_RES);

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
      if (target != NULL)
        fclose(target);
      strcpy(target_path, opts->document_root);
      strcat(target_path, "/error.html");
      target = fopen(target_path, "r");

      res->status_code = strdup("404");
      res->reason_phrase = strdup("Not Found");
    }
    if (d != NULL)
      closedir(d);

    // Http Version
    res->http_version = strdup(HTTP_VERSION);

    // Server
    map_put(res->header_map, strdup("Server"), strdup(SERVER_NAME));

    // Content-Type
    map_put(res->header_map, strdup("Content-Type"),
            strdup(get_mime_type(target_path)));

    // Body
    res->body = malloc(1024 * 64);
    int len = set_file(res->body, target);

    // Content-Length: TODO
    char *buf = malloc(6);
    sprintf(buf, "%d", len);
    map_put(res->header_map, strdup("Content-Length"), buf);

    if (target != NULL)
      fclose(target);
  } else {
    // Not Allowed
    res->status_code = strdup("405");
    res->reason_phrase = strdup("Not Allowed");
  }

  return res;
}

static char *get_mime_type(char *fname) {
  char *ext = strrchr(fname, '.');
  if (ext == NULL)
    return "text/plain";

  char *mime = map_get(Mime_map, ext + 1);
  if (mime == NULL)
    return "text/plain";

  return mime;
}

static int set_file(char *dest, FILE *f) {
  int c, len = 0;
  char *p = dest;

  if (f == NULL)
    return len;

  while ((c = fgetc(f)) != EOF) {
    *p++ = c;
    len++;
  }
  *p = '\0';

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

  req->method = strdup("FOO");
  res = create_http_response(req, opt);
  expect(__LINE__, HM_RES, res->_ty);
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

static void test_set_file() {
  char buf[2048];
  FILE *f = fopen("LICENSE", "r");
  int len = set_file(buf, f);
  fclose(f);

  expect(__LINE__, 'M', buf[0]);
  expect(__LINE__, 1064, len);
}

static void test_write_log() {
  time_t req_time = 1602737916;
  FILE *log = fopen("log", "w");
  Socket *sock = create_server_socket(8081);
  HttpMessage *req = new_HttpMessage(HM_REQ);
  req->method = strdup("GET");
  req->request_uri = strdup("/hello.html");
  req->http_version = strdup("HTTP/1.1");
  map_put(req->header_map, strdup("Referer"),
          strdup("http://localhost:8080/hello2.html"));
  map_put(req->header_map, strdup("User-Agent"), strdup("Dali/0.1"));
  HttpMessage *res = new_HttpMessage(HM_RES);
  res->status_code = strdup("200");
  map_put(res->header_map, strdup("Content-Length"), strdup("199"));

  write_log(log, sock, &req_time, req, res);
  fclose(log);

  char buf[1048];
  log = fopen("log", "r");
  set_file(buf, log);

  // clang-format off
  expect_str(__LINE__,
      "0.0.0.0 - - [15/Oct/2020:13:58:36 +0900] \"GET /hello.html HTTP/1.1\" "
      "200 199 \"http://localhost:8080/hello2.html\" \"Dali/0.1\"\n",
      buf);
  // clang-format on  
  fclose(log);
  unlink("log");
  delete_HttpMessage(req);
  delete_HttpMessage(res);  
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
  test_set_file();
  test_write_log();
}
