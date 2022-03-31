![C/C++ CI](https://github.com/youpong/chttpd/workflows/C/C++%20CI/badge.svg)

# Web Server

A multi-process HTTP 1.1 Server implemented in C,
records apache-like access logs.

This software is released under [MIT license](./LICENSE).


## RUNNING

The web server runs on Ubuntu-20.04 x86_64.

To start the server, run the following command:

```bash
$ ./httpd [-r DOCUMENT_ROOT] [-l ACCESS_LOG] [-p PORT]
```

To stop the server, just press Ctrl+C on the command line.

options:

- `-r DOCUMENT_ROOT` : set document root (default: www)

- `-l ACCESS_LOG` : set access log (default: access.log)

- `-p PORT` : listen port PORT (default: 8088)

To show the version, run the following command:

```bash
$ ./httpd -v
```

## BUILD

To build, run the following command:

```bash
$ make all
```

## TEST

To test, run the following command:

```bash
$ make check
```

## API Docs

To generate api docs, run the following command:

```bash
$ make docs
```

## Reference

*  https://www.rfc-editor.org/rfc/rfc7230.html Hypertext Transfer Protocol (HTTP/1.1): Message Syntax and Routing
*  https://httpd.apache.org/docs/2.4/en/logs.html Apache HTTP Server V2.4 Log Files

