#include "httpd.h"
#include <stdlib.h>
#include <unistd.h>

void worker_start(int sock, struct sockaddr_in *client_addr, Option *opt) {
  Logger *log;

  while (true) {
    log = (Logger *)malloc(sizeof(Logger));
    time(&log->date);
    //    setPeerAddr(log, addr);

    write(sock, "Hello", 5);
    break;
  }
}
