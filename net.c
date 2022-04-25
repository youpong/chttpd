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

/**
 * Destroy the Socket object
 *
 * @param sock the pointer to Socket
 */
void delete_Socket(Socket *sock) {
    if (sock->_ty == S_CLT) {
        fclose(sock->ips);
        fclose(sock->ops);
    }
    close(sock->_fd);

    free(sock->addr);
    free(sock);
}

/**
 * Creates a new Socket object for the server
 *
 * @return a pointer to Socket object
 * @param port number
 * @param ex a pointer to Exception
 */
Socket *new_ServerSocket(int port, Exception *ex) {
    Socket *sv_sock = new_Socket(S_SRV);

    /* create a socket, endpoint of connection */
    if ((sv_sock->_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ex->ty = E_Failure;
        ex->msg = "socket";
        return sv_sock;
    }

    /* bind */
    struct sockaddr_in *addr = sv_sock->addr;
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = INADDR_ANY;
    if (bind(sv_sock->_fd, (struct sockaddr *)addr, sv_sock->addr_len) < 0) {
        ex->ty = E_Failure;
        ex->msg = "bind";
        return sv_sock;
    }

    /* listen */
    if (listen(sv_sock->_fd, LISTEN_QUEUE) == -1) {
        ex->ty = E_Failure;
        ex->msg = "listen";
        return sv_sock;
    }

    return sv_sock;
}

/**
 * Accepts a connection on Socket object
 *
 * @return a new connected Socket object
 * @param self the pointer to Socket object
 * @param ex the pointer to Exception object
 */
Socket *ServerSocket_accept(Socket *self, Exception *ex) {
    Socket *sock = new_Socket(S_CLT);

    sock->_fd =
        accept(self->_fd, (struct sockaddr *)sock->addr, &sock->addr_len);
    if (sock->_fd < 0) {
        ex->ty = E_Failure;
        ex->msg = "accept";
        return sock;
    }

    sock->ips = fdopen(sock->_fd, "r");
    if (sock->ips == NULL) {
        ex->ty = E_Failure;
        ex->msg = "fdopen";
        return sock;
    }

    sock->ops = fdopen(sock->_fd, "w");
    if (sock->ops == NULL) {
        ex->ty = E_Failure;
        ex->msg = "fdopen";
        return sock;
    }

    return sock;
}

/**
 * Decodes an application/x-www-form-urlencoded string.
 *
 * @param dest the decoded string
 * @param src the string to decode
 *
 * @see
 * https://docs.oracle.com/en/java/javase/15/docs/api/java.base/java/net/URLDecoder.html
 * @see https://url.spec.whatwg.org/
 */
void url_decode(char *dest, const char *src) {
    const char *p = src;

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
        const char *q;
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

        int len;
    IllegalByteSequence:
        // append to dest '%' and trailing 2..0 bytes
        len = q - p + 1;
        if (*q == '\0') // not copy '\0'
            len--;

        memcpy(dest, p, len);
        dest += len;
        p += len;
    }

    *dest = '\0';
}

//
// http
//

static void request_line(FILE *f, HttpMessage *req, Exception *);
static void message_header(FILE *f, HttpMessage *req, Exception *);
static char *read_line(FILE *f);
static bool consume(FILE *f, char c);

/**
 * Create a new HttpMessage object.
 *
 * @param ty message type, request or response.
 * @return a newly created HttpMessage object.
 */
HttpMessage *new_HttpMessage(HttpMessageType ty) {
    HttpMessage *result = calloc(1, sizeof(HttpMessage));
    result->_ty = ty;
    result->header_map = new_Map();
    return result;
}

/**
 * Destroy a HttpMessage object.
 *
 * @param msg the HttpMessage to destroy
 *
 */
void delete_HttpMessage(HttpMessage *msg) {
    if (msg == NULL) {
        return;
    }

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
 * Parse a HTTP Message.
 *
 * @return a pointer to HttpMessage object.
 * @param f The input source for the HTTP Message
 * @param ty The HttpMessageType
 * @param ex The exception object
 * @param debug The debug mode
 */
HttpMessage *HttpMessage_parse(FILE *f, HttpMessageType ty, Exception *ex,
                               bool debug) {
    assert(ty == HM_REQ); // not implemented HM_RES yet.

    HttpMessage *msg = new_HttpMessage(ty);

    // parse: start-line = Request-Line | Status-Line
    switch (ty) {
    case HM_REQ:
        request_line(f, msg, ex);
        if (ex->ty != E_Okay)
            return msg;
        break;
    case HM_RES:
        break;
    }

    // parse: *(message-header CRLF) CRLF
    message_header(f, msg, ex);
    if (ex->ty != E_Okay)
        return msg;

    // parse: [message-body]
    // ...

    return msg;
}

static void request_line(FILE *f, HttpMessage *msg, Exception *ex) {
    char *p, *p0;

    assert(msg->_ty == HM_REQ);

    char *line = read_line(f);
    if (line == NULL) {
        ex->ty = HM_EmptyRequest;
        return;
    }

    // request_line
    msg->request_line = strdup(line);

    // method
    if ((p = strchr(line, ' ')) == NULL)
        goto bad_request;
    *p = '\0';
    msg->method = strdup(line);

    // request_uri
    p0 = ++p;
    if ((p = strchr(p0, ' ')) == NULL)
        goto bad_request;
    *p = '\0';
    msg->request_uri = calloc(strlen(p0) + 1, sizeof(char));
    url_decode(msg->request_uri, p0);

    // http_version
    msg->http_version = strdup(++p);

    // bad_request: empty field
    if (strlen(msg->method) == 0 || strlen(msg->request_uri) == 0 ||
        strlen(msg->http_version) == 0)
        goto bad_request;

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
    msg->filename = strdup(msg->request_uri);
    if ((p = strchr(msg->filename, '?')) != NULL)
        *p = '\0';

    // query_str
    // msg->query_str = strdup(++p);

    return;

bad_request:
    ex->ty = HM_BadRequest;
    free(line);
    return;
}

/**
 * parse
 * *(message-header CRLF) CRLF
 * message-header = field-name ":" [field-value]
 */
static void message_header(FILE *f, HttpMessage *msg, Exception *ex) {
    char *key, *value;
    char *p, *line;

    while ((line = read_line(f)) != NULL) {
        if (strlen(line) == 0)
            break;

        // key
        if ((p = strchr(line, ':')) == NULL)
            goto bad_request;
        *p++ = '\0';
        key = strdup(line);

        // consume ' '
        if (*p == ' ') {
            p++;
        }

        // value
        value = strdup(p);

        Map_put(msg->header_map, key, value);
        free(line);

        // empty key is invalid
        if (strlen(key) == 0)
            goto bad_request;
    }
    return;

bad_request:
    ex->ty = HM_BadRequest;
    free(line);
    return;
}

static char *read_line(FILE *f) {
    char *ret;
    StringBuffer *sb = new_StringBuffer();

    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\r') {
            consume(f, '\n');
            break;
        }
        StringBuffer_appendChar(sb, c);
    }
    if (c == EOF) {
        delete_StringBuffer(sb);
        return NULL;
    }

    ret = StringBuffer_toString(sb);
    delete_StringBuffer(sb);

    return ret;
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

static void test_read_line() {
    char *str = "HTTP/1.1 200 OK\r\n";
    FILE *f = tmpfile();
    fputs(str, f);
    rewind(f);
    expect_str(__LINE__, "HTTP/1.1 200 OK", read_line(f));
}

static void test_HttpMessage_parse() {
    FILE *f = tmpfile();
    HttpMessage *req;
    Exception *ex = calloc(1, sizeof(Exception));

    //
    // Normal
    //
    f = tmpfile();
    fprintf(f, "GET /hello.html HTTP/1.1\r\n"
               "Host: localhost\r\n"
               "\r\n");
    rewind(f);
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, HM_REQ, req->_ty);
    expect(__LINE__, HMMT_GET, req->method_ty);
    expect_str(__LINE__, "GET /hello.html HTTP/1.1", req->request_line);
    expect_str(__LINE__, "GET", req->method);
    expect_str(__LINE__, "/hello.html", req->request_uri);
    expect_str(__LINE__, "HTTP/1.1", req->http_version);
    expect_str(__LINE__, "/hello.html", req->filename);
    expect_str(__LINE__, "localhost", (char *)Map_get(req->header_map, "Host"));
    delete_HttpMessage(req);

    //
    // Normal(Empty message-header)
    //
    f = tmpfile();
    fprintf(f, "GET /hello.html HTTP/1.1\r\n"
               "\r\n");
    rewind(f);
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, E_Okay, ex->ty);
    expect(__LINE__, HMMT_GET, req->method_ty);
    delete_HttpMessage(req);

    //
    // Normal(HEAD)
    //
    f = tmpfile();
    fprintf(f, "HEAD /hello.html HTTP/1.1\r\n"
               "\r\n");
    rewind(f);
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, E_Okay, ex->ty);
    expect(__LINE__, HMMT_HEAD, req->method_ty);
    delete_HttpMessage(req);

    //-----------
    // Irregular
    //-----------

    //
    // empty request
    //
    f = tmpfile();
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, HM_EmptyRequest, ex->ty);

    //
    // Omitting SP in Request-Line
    //
    f = tmpfile();
    fprintf(f, "GET /hello.htmlHTTP/1.1\r\n"
               "Host: localhost\r\n"
               "\r\n");
    rewind(f);
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, HM_BadRequest, ex->ty);
    expect_str(__LINE__, "GET /hello.htmlHTTP/1.1", req->request_line);

    //
    // empty Method
    //
    f = tmpfile();
    fprintf(f, " /hello.html HTTP/1.1\r\n"
               "Host: localhost\r\n"
               "\r\n");
    rewind(f);
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, HM_BadRequest, ex->ty);

    //
    // Empty key in message-header
    //
    f = tmpfile();
    fprintf(f, "GET /hello.html HTTP/1.1\r\n"
               ": localhost\r\n"
               "\r\n");
    rewind(f);
    req = HttpMessage_parse(f, HM_REQ, ex, false);
    fclose(f);
    expect(__LINE__, HM_BadRequest, ex->ty);
    expect(__LINE__, HMMT_GET, req->method_ty);
}

void run_all_test_net() {
    test_url_decode();
    test_read_line();
    test_HttpMessage_parse();
}
