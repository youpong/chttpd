#include "file.h"
#include "main.h"
#include "net.h"
#include "util.h"

#include <arpa/inet.h> // inet_ntoa(3)
#include <errno.h>     // errno
#include <fcntl.h>     // open(2)
#include <stdlib.h>    // malloc(3)
#include <string.h>    // strcmp(3), strrchr(3)
#include <sys/stat.h>  // open(2)
#include <time.h>      // time(2)
#include <unistd.h>    // unlink(2)

static void header_put(HttpMessage *msg, char *key, char *value);
static char *header_get(HttpMessage *msg, char *key, char *default_val);
static File *new_File2(char *parent_path, char *child_path);

static void handle_connection(Socket *sock, FILE *log, Option *opt);
static HttpMessage *new_HttpResponse(HttpMessage *, Option *, Exception *ex);
static void write_log(FILE *, Socket *, time_t *, HttpMessage *, HttpMessage *);

void server_start(Option *opt) {
  Exception *ex = calloc(1, sizeof(Exception));

  FILE *log = fopen(opt->access_log, "a");
  if (log == NULL) {
    perror("fopen");
    exit(1);
  }

  Socket *sv_sock = new_ServerSocket(opt->port, ex);
  if (sv_sock == NULL)
    error("Error: new_ServerSock: %s: %s", ex->msg, strerror(errno));
  printf("listen: %s:%d\n", inet_ntoa(sv_sock->addr->sin_addr),
         ntohs(sv_sock->addr->sin_port));

  for (int i = 0; i < MAX_SERVERS; i++) {
    pid_t pid = fork();
    switch (pid) {
    case -1: // error
      perror("fork");
      exit(1);
    case 0: // child
      while (true) {
        Socket *sock = ServerSocket_accept(sv_sock, ex);
        if (sock == NULL)
          error("Error: ServerSock_accept: %s: %s", ex->msg, strerror(errno));
        printf("open pid: %d, address: %s, port: %d\n", getpid(),
               inet_ntoa(sock->addr->sin_addr), ntohs(sock->addr->sin_port));
        handle_connection(sock, log, opt);
        delete_Socket(sock);
      }
      break;
    default: // parent
             /* no-op */;
    }
  }

  pause();

  fclose(log);
  delete_Socket(sv_sock);
  free(ex);
}

static void handle_connection(Socket *sock, FILE *log, Option *opt) {
  Exception *ex = calloc(1, sizeof(Exception));
  time_t req_time;
  bool conn_keep_alive = true;

  while (conn_keep_alive) {
    time(&req_time);

    HttpMessage *req = HttpMessage_parse(sock->ips, HM_REQ, ex, opt->debug);
    if (ex->ty == HM_EmptyRequest)
      break;

    HttpMessage *res = new_HttpResponse(req, opt, ex);

    HttpMessage_write(res, sock->ops);
    write_log(log, sock, &req_time, req, res);

    if (strcmp(header_get(req, "Connection", ""), "close") == 0 ||
        strcmp(header_get(res, "Connection", ""), "close") == 0)
      conn_keep_alive = false;

    delete_HttpMessage(req);
    delete_HttpMessage(res);
  }
  free(ex);
}

static int file_read(File *file, char *dest); // extern ?
static char *get_mime_type(char *fname);

// TODO: 404 handle error if error.html is not found
static HttpMessage *new_HttpResponse(HttpMessage *req, Option *opts,
                                     Exception *ex) {
  HttpMessage *res = new_HttpMessage(HM_RES);
  File *file;
  char buf[20 + 1]; // log10(ULONG_MAX) < 20

  if (ex->ty != E_Okay) {
    res->http_version = strdup(HTTP_VERSION);
    res->status_code = strdup("400");
    res->reason_phrase = strdup("Bad Request");
    header_put(res, "Server", SERVER_NAME);
    header_put(res, "Content-Type", "text/html");
    // TODO: system information, server name and os name
    res->body = strdup("<html>\n"
                       "<head><title>400 Bad Request</title></head>\n"
                       "<body>\n"
                       "<center><h1>400 Bad Request</h1></center>\n"
                       "</body>\n"
                       "</html>\n");
    res->body_len = strlen(res->body);
    sprintf(buf, "%d", res->body_len);
    header_put(res, "Content-Length", buf);
    header_put(res, "Connection", "close");
    return res;
  }

  switch (req->method_ty) {
  case HMMT_GET:
  case HMMT_HEAD:
    // HTTP-Version
    res->http_version = strdup(HTTP_VERSION);

    // Status-Code, Reason-Phrase
    file = new_File2(opts->document_root, req->filename);
    if (file != NULL && file->ty == F_FILE) {
      res->status_code = strdup("200");
      res->reason_phrase = strdup("OK");
    } else {
      file = new_File2(opts->document_root, "/error.html");
      res->status_code = strdup("404");
      res->reason_phrase = strdup("Not Found");
    }

    // Server
    header_put(res, "Server", SERVER_NAME);

    // Content-Type
    header_put(res, "Content-Type", get_mime_type(file->path));

    // Content-Length
    sprintf(buf, "%d", file->len);
    header_put(res, "Content-Length", buf);

    // Body (omit if POST method)
    if (req->method_ty == HMMT_GET) {
      res->body = malloc(file->len + 1);
      res->body_len = file_read(file, res->body);
    }

    delete_File(file);
    break;
  default:
    // Not Allowed Request method
    res->status_code = strdup("405");
    res->reason_phrase = strdup("Not Allowed");
  }

  return res;
}

static void header_put(HttpMessage *msg, char *key, char *value) {
  Map_put(msg->header_map, strdup(key), strdup(value));
}

static char *header_get(HttpMessage *msg, char *key, char *default_val) {
  char *val;

  if (msg == NULL)
    return default_val;
  val = Map_get(msg->header_map, key);
  if (val == NULL)
    return default_val;
  return val;
}

static File *new_File2(char *parent_path, char *child_path) {
  File *file;
  StringBuffer *sb = new_StringBuffer();
  char *path;

  StringBuffer_append(sb, parent_path);
  StringBuffer_append(sb, child_path);
  path = StringBuffer_toString(sb);

  file = new_File(path);

  free(path);
  delete_StringBuffer(sb);

  return file;
}

static char *get_mime_type(char *path) {
  char *ext = extension(path);
  if (ext == NULL)
    return "text/plain";

  char *mime = Map_get(MimeMap, ext);

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

  char *buf;
  // clang-format off
  fprintf(out, "%s - - [%s] \"%s\" %s %s \"%s\" \"%s\"\n",
	  inet_ntoa(sock->addr->sin_addr),
	  buf = formatted_time(&req_tm, timezone),
	  req->request_line,
	  res->status_code,
	  header_get(res, "Content-Length", "\"-\""),	  
	  header_get(req, "Referer", "-"),
	  header_get(req, "User-Agent", "-"));
  // clang-format on
  fflush(out);

  unlock();
  free(buf);
}

/**
 * e.g.
 * "09/Oct/2020:17:34:23 +0900"
 * storage duration: dynamic
 * caller frees allocated memory for result.
 */
static char *formatted_time(struct tm *t_tm, long timezone) {
  char date[20 + 1];
  char *buf = malloc(26 + 1);

  strftime(date, 20 + 1, "%d/%b/%Y:%H:%M:%S", t_tm);
  sprintf(buf, "%s %+03d%02d", date,
          (int)-timezone / (60 * 60),      // hour of timezone
          abs(timezone) % (60 * 60) / 60); // minute of timezone
  return buf;
}

static bool is_locked() {
  int fd;
  if ((fd = open("/var/lock/dali.pid", O_RDONLY)) != -1) {
    close(fd);
    return true;
  }

  return false;
}

static int lock() {
  int fd = open("/var/lock/dali.pid", O_CREAT | O_EXCL);
  close(fd);
  return fd;
}

static void unlock() {
  unlink("/var/lock/dali.pid");
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

  t = 1602589880;
  gmtime_r(&t, &t_tm);
  expect_str(__LINE__, "13/Oct/2020:11:51:20 +0900",
             formatted_time(&t_tm, -(9 * 60 * 60)));
}

static void test_new_HttpResponse() {
  HttpMessage *res;
  HttpMessage *req = new_HttpMessage(HM_REQ);
  Option *opt = malloc(sizeof(Option));
  opt->document_root = strdup("www");
  Exception *ex = calloc(1, sizeof(Exception));

  // Not Allowed Request method
  req->method = strdup("FOO");
  req->method_ty = HMMT_UNKNOWN;
  res = new_HttpResponse(req, opt, ex);
  expect(__LINE__, HM_RES, res->_ty);
  expect_str(__LINE__, "405", res->status_code);

  // GET not exist filename
  req->method = strdup("GET");
  req->method_ty = HMMT_GET;
  req->request_uri = strdup("/not_exist");
  req->filename = strdup("/not_exist");
  res = new_HttpResponse(req, opt, ex);
  expect_str(__LINE__, "404", res->status_code);
  expect_bool(__LINE__, true,
              res->body_len == atoi(header_get(res, "Content-Length", "")));

  // HEAD not exist filename
  req->method = strdup("HEAD");
  req->method_ty = HMMT_HEAD;
  req->request_uri = strdup("/not_exist");
  req->filename = strdup("/not_exist");
  res = new_HttpResponse(req, opt, ex);
  expect_str(__LINE__, "404", res->status_code);
  expect_ptr(__LINE__, NULL, res->body);

  // GET
  req->method = strdup("GET");
  req->method_ty = HMMT_GET;
  req->request_uri = strdup("/hello.html");
  req->filename = strdup("/hello.html");
  res = new_HttpResponse(req, opt, ex);
  expect_str(__LINE__, "200", res->status_code);
  expect_bool(__LINE__, true,
              res->body_len == atoi(header_get(res, "Content-Length", "")));

  // HEAD
  req->method = strdup("HEAD");
  req->method_ty = HMMT_HEAD;
  req->request_uri = strdup("/hello.html");
  req->filename = strdup("/hello.html");
  res = new_HttpResponse(req, opt, ex);
  expect_str(__LINE__, "200", res->status_code);
  expect_ptr(__LINE__, NULL, res->body);

  free(ex);
}

static void test_file_read() {
  File *file = new_File("LICENSE");

  char *buf = malloc(file->len);
  int len = file_read(file, buf);

  expect(__LINE__, 'M', buf[0]);
  expect(__LINE__, 1064, len);
  delete_File(file);
}

static void test_write_log() {
  Exception *ex = calloc(1, sizeof(Exception));
  //
  // write log
  //
  Socket *sock = new_ServerSocket(8081, ex);

  HttpMessage *req = new_HttpMessage(HM_REQ);
  req->request_line = strdup("GET /hello.html HTTP/1.1");
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

  File *f = new_File("log");
  char *buf = malloc(f->len);
  file_read(f, buf);
  delete_File(f);
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

  free(ex);
}

static void test_get_mime_type() {
  // clang-format off
  expect_str(__LINE__, "text/html",  get_mime_type("www/index.html"));
  expect_str(__LINE__, "text/html",  get_mime_type("/index.html"));  
  expect_str(__LINE__, "text/plain", get_mime_type("www/extless"));
  expect_str(__LINE__, "text/plain", get_mime_type("a.unknownext"));
  expect_str(__LINE__, "text/plain", get_mime_type(".dotfile"));
  // clang-format on  
}

void run_all_test_server() {
  test_get_mime_type();
  test_formatted_time();
  test_new_HttpResponse();
  test_file_read();
  test_write_log();
}
