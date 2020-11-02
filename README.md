# Web Server

A simple multi-process HTTP 1.1 Server implemented in C.

This software is released under MIT license.


## RUNNING

The web server runs on Ubuntu-20.

To start the server, run the following command:

```bash
$ ./httpd [-r DOCUMENT_ROOT] [PORT]
```

To stop the server, just press Ctrl+C on the command line.

options:

- `-r DOCUMENT_ROOT` : set document root (default: www)

- `-l ACCESS_LOG` : set access log (default: access.log)

arguments:

- `PORT` : listen port PORT (default: 8088)

## BUILD

Run the following command:

```bash
$ make all
```

## TEST

Run the following command:

```bash
$ make check
```




