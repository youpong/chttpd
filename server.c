/**
 * @file server.c
 */
#include "file.h"
#include "main.h"
#include "net.h"
#include "util.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

pid_t Pids[MAX_SERVERS];

static void cleanup(int);
static void header_put(HttpMessage *msg, char *key, char *value);
static char *header_get(HttpMessage *msg, char *key, char *default_val);
static File *new_File2(char *parent_path, char *child_path);

static void handle_connection(Socket *sock, FILE *log, Option *opt);
static HttpMessage *new_HttpResponse(HttpMessage *, Option *, Exception *ex);
static HttpMessage *new_HttpResponse_for_bad_query(HttpMessage *, Option *,
                                                   Exception *);

static void write_msg(HttpMessage *, HttpMessage *, FILE *);
static int write_log(FILE *, Socket *, time_t *, HttpMessage *, HttpMessage *);

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
      Pids[i] = pid;
    }
  }

  signal(SIGTERM, cleanup);

  for (int i = 0; i < MAX_SERVERS; ++i) {
    int wstatus;
    waitpid(-1, &wstatus, 0);
  }

  fclose(log);
  delete_Socket(sv_sock);
  free(ex);
}

static void cleanup(int sig_type) {
  signal(sig_type, SIG_DFL);

  for (int i = 0; i < MAX_SERVERS; ++i) {
    kill(Pids[i], sig_type);
  }
}

static void handle_connection(Socket *sock, FILE *log, Option *opt) {
  HttpMessage *req, *res;
  Exception *ex = calloc(1, sizeof(Exception));

  bool cond = true;
  while (cond) {
    time_t req_time;
    time(&req_time);

    req = HttpMessage_parse(sock->ips, HM_REQ, ex, opt->debug);

    if (ex->ty == E_Okay)
      res = new_HttpResponse(req, opt, ex);
    else if (ex->ty == HM_EmptyRequest) {
      cond = false;
      res = NULL;
      goto cleanup;
    } else {
      // todo
      res = new_HttpResponse_for_bad_query(req, opt, ex);
    }

    write_msg(req, res, sock->ops);
    write_log(log, sock, &req_time, req, res);

    if (strcmp(header_get(req, "Connection", ""), "close") == 0 ||
        strcmp(header_get(res, "Connection", ""), "close") == 0) {
      cond = false;
    }

  cleanup:
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

  switch (req->method_ty) {
  case HMMT_GET:
  case HMMT_HEAD:
    // HTTP-Version
    res->http_version = strdup(HTTP_VERSION);

    // Server
    header_put(res, "Server", SERVER_NAME);

    // Status-Code, Reason-Phrase
    file = new_File2(opts->document_root, req->filename);
    if (file != NULL && file->ty == F_FILE) {
      res->status_code = strdup("200");
      res->reason_phrase = strdup("OK");
    } else {
      res->status_code = strdup("404");
      res->reason_phrase = strdup("Not Found");
      header_put(res, "Content-Type", "text/html");
      res->body = strdup("<html>\n"
                         "<head><title>404 Not found</title></head>\n"
                         "<body>\n"
                         "<center><h1>404 Not found</h1></center>\n"
                         "</body>\n"
                         "</html>\n");
      res->body_len = strlen(res->body);
      if (req->method_ty == HMMT_HEAD) {
        free(res->body);
        res->body = NULL;
      }
      sprintf(buf, "%d", res->body_len);
      header_put(res, "Content-Length", buf);
      return res;
    }

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

static HttpMessage *
new_HttpResponse_for_bad_query(HttpMessage *req, Option *opts, Exception *ex) {
  HttpMessage *res = new_HttpMessage(HM_RES);
  char buf[20 + 1]; // log10(ULONG_MAX) < 20

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

static void write_msg(HttpMessage *req, HttpMessage *res, FILE *f) {
  assert(req->_ty == HM_REQ);
  assert(res->_ty == HM_RES);

  // status_line
  fprintf(f, "%s %s %s\r\n", res->http_version, res->status_code,
          res->reason_phrase);

  // headers
  Map *map = res->header_map;
  for (int i = 0; i < map->keys->len; i++) {
    fprintf(f, "%s: %s\r\n", (char *)map->keys->data[i],
            (char *)map->vals->data[i]);
  }

  // CRLF
  fprintf(f, "\r\n");

  if (req->method_ty != HMMT_HEAD)
    for (int i = 0; i < res->body_len; i++) {
      fputc(res->body[i], f);
    }

  fflush(f);
}

static int write_log(FILE *out, Socket *sock, time_t *req_time,
                     HttpMessage *req, HttpMessage *res) {
  struct tm req_tm;
  int size;
  localtime_r(req_time, &req_tm);

  do {
    while (is_locked()) {
      sleep(1);
    }
  } while (lock() == -1);

  char *buf;
  // clang-format off
  size = fprintf(out, "%s - - [%s] \"%s\" %s %s \"%s\" \"%s\"\n",
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

  return size;
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
          (int)-timezone / (60 * 60),           // hour of timezone
          abs((int)timezone) % (60 * 60) / 60); // minute of timezone
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

  free(opt);
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
  //
  // write log
  //
  Exception *ex = calloc(1, sizeof(Exception));
  Socket *sock = new_ServerSocket(8081, ex);

  HttpMessage *req = new_HttpMessage(HM_REQ);
  req->request_line = strdup("GET /hello.html HTTP/1.1");
  header_put(req, "Referer", "http://localhost:8080/hello2.html");
  header_put(req, "User-Agent", "Dali/0.1");

  HttpMessage *res = new_HttpMessage(HM_RES);
  res->status_code = strdup("200");
  header_put(res, "Content-Length", "199");

  time_t req_time = 1602737916;
  FILE *log = tmpfile();
  int len = write_log(log, sock, &req_time, req, res);
  delete_HttpMessage(req);
  delete_HttpMessage(res);

  //
  // read log
  //
  rewind(log);
  char *buf = malloc(len - strlen("\n") + 1);
  fgets(buf, len - strlen("\n") + 1, log);
  fclose(log);

  //
  // check
  //

  // clang-format off
  char *expected =
    "0.0.0.0 "                      // client ip addr
    "- "                            // client identity(identd)
    "- "                            // user id
    "[15/Oct/2020:13:58:36 +0900] " // request accept time
    "\"GET /hello.html HTTP/1.1\" " // request-line
    "200 "                          // Status-Code
    "199 "                          // content length
    "\"http://localhost:8080/hello2.html\" " // Referer
    "\"Dali/0.1\"";                // User-Agent
  // clang-format on
  expect(__LINE__, strlen(expected), strlen(buf));
  int i = 0;
  for (; expected[i] != '['; i++)
    expect(__LINE__, expected[i], buf[i]);
  // skip between '[' and ']'
  for (; expected[i] != ']'; i++)
    ;
  for (; expected[i] != '\0'; i++)
    expect(__LINE__, expected[i], buf[i]);

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
