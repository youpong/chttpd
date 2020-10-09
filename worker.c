#include "main.h"
#include "net.h"
#include <stdlib.h> // malloc()
#include <string.h> // strcmp()
#include <unistd.h> // write()

static HttpResponse *create_http_response(HttpRequest *);

void worker_start(Socket *sock, Option *opt) {

  while (true) {
    HttpRequest *req = http_request_parse(sock->fd, opt->debug);
    HttpResponse *res = create_http_response(req);
    write_http_response(sock->fd, res);
    write_log(stdout, sock, req, res);

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
