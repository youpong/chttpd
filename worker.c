#include "main.h"
#include "net.h"
#include <arpa/inet.h> // inet_ntoa()
#include <dirent.h>    // opendir()
#include <stdlib.h>    // malloc()
#include <string.h>    // strcmp()
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
    if (target == NULL || d != NULL) {
      target = fopen("error.html", "r");
      res->status_code = strdup("404");
      res->reason_phrase = strdup("Not Found");
    } else {
      res->status_code = strdup("200");
      res->reason_phrase = strdup("OK");
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
    printf("Unknown method: %s", req->method);
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
static char *formatted_time(time_t *t);

static void write_log(FILE *out, Socket *sock, HttpRequest *req,
                      HttpResponse *res) {
  time_t req_time;
  time(&req_time);

  // clang-format off
  fprintf(out, "%s - - [%s] \"%s %s %s\" %s %s \"%s\" \"%s\"\n",
      inet_ntoa(sock->addr->sin_addr),
      // date [09/Oct/2020:13:17:45 +0900]
      formatted_time(&req_time),
      // request-line
      req->method, req->request_uri, req->http_version,
      res->status_code,
      default_val((char *)map_get(res->header_map, "Content-Length"), "\"-\""),
      default_val((char *)map_get(req->header_map, "Referer"), "-"),
      default_val((char *)map_get(req->header_map, "User-Agent"), "-"));
  // clang-format on
  fflush(out);
}

/**
 * 09/Oct/2020:17:34:23 +0900
 */
static char *formatted_time(time_t *t) {
  char buf[256];
  char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  struct tm *t_tm = malloc(sizeof(struct tm));

  tzset();
  localtime_r(t, t_tm);

  // clang-format off
  sprintf(buf, "%02d/%s/%d:%02d:%02d:%02d %+03d%02d",
          t_tm->tm_mday,
          month[t_tm->tm_mon],
          t_tm->tm_year + 1900,
          t_tm->tm_hour,
          t_tm->tm_min,
          t_tm->tm_sec,	  
          (int)-timezone / 60 / 60,
          (int)-timezone % (60 * 60));
  // clang-format off
  
  return strdup(buf);
}

static char *default_val(char *val, char *dval) {
  return val ? val : dval;
}

/**
 * timezone -0900, 0500
 */
void test_formatted_time() {
  time_t t;
  time(&t);
  printf("%s\n", formatted_time(&t));
}

void test_create_http_response() {
  HttpRequest *req = malloc(sizeof(HttpResponse));
  req->header_map = new_map();
  Option *opt = malloc(sizeof(Option));

  req->method = strdup("GET");
  req->request_uri = strdup("main.h");
  HttpResponse *res = create_http_response(req, opt);
  write_http_response(1, res);
}

void test_set_file() {
}
