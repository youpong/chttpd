#include "main.h"
#include "net.h"
#include <arpa/inet.h> // inet_ntoa()
#include <stdlib.h>    // malloc()
#include <string.h>    // strcmp()
#include <time.h>      // time()
#include <unistd.h>    // write()

static HttpResponse *create_http_response(HttpRequest *);
static void write_log(FILE *, Socket *, HttpRequest *, HttpResponse *);

void worker_start(Socket *sock, FILE *log, Option *opt) {

  while (true) {
    HttpRequest *req = http_request_parse(sock->fd, opt->debug);
    HttpResponse *res = create_http_response(req);
    write_http_response(sock->fd, res);
    write_log(log, sock, req, res);

    if (strcmp("close", map_get(req->header_map, "Connection")) == 0)
      break;
  }
}

static HttpResponse *create_http_response(HttpRequest *req) {
  HttpResponse *res = malloc(sizeof(HttpResponse));
  res->header_map = new_map();

  if (strcmp(req->method, "GET") == 0 || strcmp(req->method, "HEAD") == 0) {
    // Http Version
    res->http_version = strdup(HTTP_VERSION);

    // Status Code
    res->status_code = strdup("200");

    // Reason Phrase
    res->reason_phrase = strdup("OK");

    // Server
    map_put(res->header_map, "Server", SERVER_NAME);

    // Content-Type
    map_put(res->header_map, "Content-Type", "text/html");

    // Body
    res->body = strdup("<html>hello</html>");

    // Content-Length: TODO
    map_put(res->header_map, "Content-Length", "18");
  }

  return res;
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
      default_val((char *)map_get(req->header_map, "Refer"), "-"),
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

  sprintf(buf, "%02d/%s/%d:%02d:%02d:%02d %+03d%02d", t_tm->tm_mday,
          month[t_tm->tm_mon], t_tm->tm_year + 1900, t_tm->tm_hour,
          t_tm->tm_min, t_tm->tm_sec, (int)-timezone / 60 / 60,
          (int)-timezone % (60 * 60));

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
