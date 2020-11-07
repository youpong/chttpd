#include "net.h"
#include "util.h"

#include <assert.h>    // assert(3)
#include <fcntl.h>     // open(2)
#include <stdlib.h>    // malloc(3)
#include <string.h>    // strdup(3)
#include <sys/stat.h>  // oepn(2)
#include <sys/types.h> // open(2)
#include <unistd.h>    // unlink(2)

//
// general net
//

static Socket *new_Socket(SocketType ty) {
  Socket *sock = calloc(1, sizeof(Socket));
  sock->_ty = ty;
  sock->addr = calloc(1, sizeof(struct sockaddr_in));
  sock->addr_len = sizeof(*sock->addr);

  return sock;
}

void delete_Socket(Socket *sock) {
  if (sock->_ty == S_CLT) {
    fclose(sock->ips);
    fclose(sock->ops);
  }
  close(sock->_fd);

  free(sock->addr);
  free(sock);
}

Socket *new_ServerSocket(int port) {
  Socket *sv_sock = new_Socket(S_SRV);

  /* create a socket, endpoint of connection */
  if ((sv_sock->_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    ErrorMsg = "socket";
    return NULL;
  }

  /* bind */
  struct sockaddr_in *addr = sv_sock->addr;
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  addr->sin_addr.s_addr = INADDR_ANY;
  if (bind(sv_sock->_fd, (struct sockaddr *)addr, sv_sock->addr_len) < 0) {
    ErrorMsg = "bind";
    return NULL;
  }

  /* listen */
  if (listen(sv_sock->_fd, LISTEN_QUEUE) == -1) {
    ErrorMsg = "listen";
    return NULL;
  }

  return sv_sock;
}

Socket *ServerSocket_accept(Socket *sv_sock) {
  Socket *sock = new_Socket(S_CLT);

  if ((sock->_fd = accept(sv_sock->_fd, (struct sockaddr *)sock->addr,
                          &sock->addr_len)) < 0) {
    ErrorMsg = "accept";
    return NULL;
  }
  if ((sock->ips = fdopen(sock->_fd, "r")) == NULL) {
    ErrorMsg = "fdopen";
    return NULL;
  }
  if ((sock->ops = fdopen(sock->_fd, "w")) == NULL) {
    ErrorMsg = "fdopen";
    return NULL;
  }

  return sock;
}

/*
 * ReferTo:
 * -
 * https://docs.oracle.com/en/java/javase/15/docs/api/java.base/java/net/URLDecoder.html
 * - https://url.spec.whatwg.org/
 */
void url_decode(char *dest, char *src) {
  char *p = src;

  while (*p) {
    if (*p == '+') {
      *dest++ = ' ';
      p++;
      continue;
    }
    if (*p != '%') {
      *dest++ = *p++;
      continue;
    }

    //
    // decode sequence of form "%xy".
    //

    int n = 0;
    char *q;
    for (q = p + 1; q - p < 3; q++) {
      int d;
      if ('0' <= *q && *q <= '9')
        d = *q - '0';
      else if ('A' <= *q && *q <= 'F')
        d = *q - 'A' + 10;
      else if ('a' <= *q && *q <= 'f')
        d = *q - 'a' + 10;
      else { // include *q == '\0'
        goto IllegalByteSequence;
      }
      n = 16 * n + d;
    }

    *dest++ = n;
    p += 3;
    continue;

  IllegalByteSequence : { // clang-format off
      // append to dest '%' and trailing 2..0 bytes
      int len = q - p + 1;
      if (*q == '\0') // not copy '\0'
	len--;

      memcpy(dest, p, len);
      dest += len;
      p += len;
    }
    // clang-format on
  }

  *dest = '\0';
}

//
// http
//

static bool consume(FILE *f, char c);
static bool request_line(FILE *f, HttpMessage *req);
static bool message_header(FILE *f, HttpMessage *req);

HttpMessage *new_HttpMessage(HttpMessageType ty) {
  HttpMessage *result = calloc(1, sizeof(HttpMessage));
  result->_ty = ty;
  result->header_map = new_Map();
  return result;
}

void delete_HttpMessage(HttpMessage *msg) {
  // start-line(Request-Line|Status-Line)
  free(msg->method);
  free(msg->request_uri);
  free(msg->http_version);
  free(msg->status_code);
  free(msg->reason_phrase);

  // message-header
  delete_Map(msg->header_map);

  // message-body
  free(msg->body);

  // utilities
  free(msg->filename);

  // message
  free(msg);
}

/**
 * @return a pointer to HttpRequest object.
 * @return NULL when read null request.
 *
 * Refer to document for declaration of typedef HttpMessage.
 */
HttpMessage *HttpMessage_parse(FILE *f, HttpMessageType ty, bool debug) {
  assert(ty == HM_REQ); // not implemented HM_RES yet.

  HttpMessage *msg = new_HttpMessage(ty);

  // parse start-line = Request-Line | Status-Line
  switch (ty) {
  case HM_REQ:
    if (!request_line(f, msg))
      goto exception;
    break;
  case HM_RES:
    // if (! status_line(f, msg))
    //   goto exception;
    break;
  }

  // parse *(message_header CRLF)
  if (!message_header(f, msg))
    goto exception;

  // parse CRLF
  consume(f, '\r');
  consume(f, '\n');

  // parse [message-body]
  // if (! message_body(f, msg))
  //  goto exception;

  return msg;

exception:
  delete_HttpMessage(msg);
  return NULL;
}

static bool request_line(FILE *f, HttpMessage *msg) {
  StringBuffer *sb;
  int c;

  assert(msg->_ty == HM_REQ);

  // method
  sb = new_StringBuffer();
  while ((c = fgetc(f)) != EOF) {
    if (c == ' ')
      break;
    if (c == '\r')
      return false;
    StringBuffer_appendChar(sb, c);
  }
  msg->method = StringBuffer_toString(sb);
  delete_StringBuffer(sb);

  // request_uri
  sb = new_StringBuffer();
  while ((c = fgetc(f)) != EOF) {
    if (c == ' ')
      break;
    if (c == '\r')
      return false;
    StringBuffer_appendChar(sb, c);
  }
  char *str = StringBuffer_toString(sb);
  msg->request_uri = malloc(sb->len + 1);
  url_decode(msg->request_uri, str);
  free(str);
  delete_StringBuffer(sb);

  // http_version
  sb = new_StringBuffer();
  while ((c = fgetc(f)) != EOF) {
    if (c == '\r') {
      consume(f, '\n');
      break;
    }
    StringBuffer_appendChar(sb, c);
  }
  msg->http_version = StringBuffer_toString(sb);
  delete_StringBuffer(sb);

  // return NULL for invalid input.
  if (strlen(msg->method) == 0 || strlen(msg->request_uri) == 0 ||
      strlen(msg->http_version) == 0)
    return false;

  //
  // set members
  //

  // method type
  if (strcmp(msg->method, "GET") == 0)
    msg->method_ty = HMMT_GET;
  else if (strcmp(msg->method, "HEAD") == 0)
    msg->method_ty = HMMT_HEAD;
  else
    msg->method_ty = HMMT_UNKNOWN;

  // filename
  msg->filename = malloc(strlen(msg->request_uri) + 1);
  char *p = msg->filename;
  char *q = msg->request_uri;
  while (*q != '\0' && *q != '?')
    *p++ = *q++;
  *p = '\0';

  return true;
}

static bool message_header(FILE *f, HttpMessage *msg) {
  StringBuffer *sb;
  char *key, *value;
  int c;

  while ((c = fgetc(f)) != EOF) {
    if (c == '\r') {
      ungetc(c, f);
      break;
    }
    ungetc(c, f);

    // key
    sb = new_StringBuffer();
    while ((c = fgetc(f)) != EOF) {
      if (c == ':') {
        consume(f, ' ');
        break;
      }
      StringBuffer_appendChar(sb, c);
    }
    key = StringBuffer_toString(sb);
    delete_StringBuffer(sb);
    if (strlen(key) == 0)
      return false;

    // value
    sb = new_StringBuffer();
    while ((c = fgetc(f)) != EOF) {
      if (c == '\r') {
        consume(f, '\n');
        break;
      }
      StringBuffer_appendChar(sb, c);
    }
    value = StringBuffer_toString(sb);
    delete_StringBuffer(sb);

    Map_put(msg->header_map, key, value);

    // empty key is invalid
    if (strlen(key) == 0)
      return NULL;
  }

  return true;
}

static bool consume(FILE *f, char expected) {
  int c;

  c = fgetc(f);
  if (c != expected) {
    ungetc(c, f);
    return false;
  }

  return true;
}

void HttpMessage_write(HttpMessage *msg, FILE *f) {
  assert(msg->_ty == HM_RES); // HM_REQ not implemented yet.

  switch (msg->_ty) {
  case HM_REQ:
    break;
  case HM_RES:
    // status_line
    fprintf(f, "%s %s %s\r\n", msg->http_version, msg->status_code,
            msg->reason_phrase);
  }

  // headers
  Map *map = msg->header_map;
  for (int i = 0; i < map->keys->len; i++) {
    fprintf(f, "%s: %s\r\n", (char *)map->keys->data[i],
            (char *)map->vals->data[i]);
  }

  // CRLF
  fprintf(f, "\r\n");

  // body(option)
  for (int i = 0; i < msg->body_len; i++) {
    fputc(msg->body[i], f);
  }

  fflush(f);
}

static void test_url_decode() {
  char buf[100];

  // normal case 1
  url_decode(buf, "abc");
  expect_str(__LINE__, "abc", buf);

  // normal case 2
  url_decode(buf, "a+%40%3A%3bz");
  expect_str(__LINE__, "a @:;z", buf);

  // minimal case
  url_decode(buf, "a");
  expect_str(__LINE__, "a", buf);

  // empty case
  url_decode(buf, "");
  expect_str(__LINE__, "", buf);

  // special characters remain the same
  url_decode(buf, "-_.*");
  expect_str(__LINE__, "-_.*", buf);

  // '+' is converted into a space characer
  url_decode(buf, "+");
  expect_str(__LINE__, " ", buf);

  //
  // leave illegal byte sequence alone.
  //

  // invalid hex
  url_decode(buf, "%3G%G3");
  expect_str(__LINE__, "%3G%G3", buf);

  // empty trailing
  url_decode(buf, "%");
  expect_str(__LINE__, "%", buf);

  // shortage trailing
  url_decode(buf, "%3");
  expect_str(__LINE__, "%3", buf);
}

static void test_HttpMessage_parse() {
  FILE *f = tmpfile();
  fprintf(f, "GET /hello.html HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "\r\n");
  rewind(f);
  HttpMessage *req = HttpMessage_parse(f, HM_REQ, false);
  fclose(f);

  expect(__LINE__, HM_REQ, req->_ty);
  expect_str(__LINE__, "GET", req->method);
  expect_str(__LINE__, "/hello.html", req->request_uri);
  expect_str(__LINE__, "HTTP/1.1", req->http_version);
  expect_str(__LINE__, "/hello.html", req->filename);
  expect_str(__LINE__, "localhost", (char *)Map_get(req->header_map, "Host"));
  delete_HttpMessage(req);

  //
  // empty request
  //
  f = tmpfile();
  req = HttpMessage_parse(f, HM_REQ, false);
  fclose(f);

  expect_ptr(__LINE__, NULL, req);

  //
  // empty Http-Version
  //
  f = tmpfile();
  fprintf(f, "GET /hello.html\r\n"
             "Host: localhost\r\n"
             "\r\n");
  rewind(f);
  req = HttpMessage_parse(f, HM_REQ, false);
  fclose(f);

  expect_ptr(__LINE__, NULL, req);
}

static void test_HttpMessage_write() {
  HttpMessage *res = new_HttpMessage(HM_RES);

  res->http_version = strdup("HTTP/1.1");
  res->status_code = strdup("200");
  res->reason_phrase = strdup("OK");

  Map_put(res->header_map, strdup("Server"), strdup("Dali/0.1"));
  Map_put(res->header_map, strdup("Content-Length"), strdup("4"));

  res->body_len = 4;
  res->body = strdup("body");

  FILE *f = tmpfile();
  HttpMessage_write(res, f);
  rewind(f);

  // clang-format off
  char *p = "HTTP/1.1 200 OK\r\n"
             "Server: Dali/0.1\r\n"
             "Content-Length: 4\r\n"
             "\r\n"
             "body";
  // clang-format on

  int c;
  while ((c = fgetc(f)) != EOF) {
    expect(__LINE__, *p++, c);
  }
  expect(__LINE__, '\0', *p);

  delete_HttpMessage(res);
}

void run_all_test_net() {
  test_url_decode();
  test_HttpMessage_parse();
  test_HttpMessage_write();
}
