#include "net.h"
#include "main.h"
#include <unistd.h> // write

void worker_start(Socket *sock, Option *opt) {

  while (true) {
    HttpRequest *req = http_request_parse(sock->fd, opt->debug);
    HttpResponse *res = create_http_response(req);
    write_http_response(sock->fd, res);
    write_log(stdout, sock, req, res);
    write(sock->fd, "Hello", 5); // for debug

    //    if (strcmp("close", get_header(req->header, "Connection")) == 0)
    break;
  }
}

