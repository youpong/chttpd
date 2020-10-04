#include "httpd.h"
#include <stdlib.h>
#include <unistd.h>

void worker_start(int sock, Option *opt) {
  Logger *log;

  while (true) {
    log = (Logger *)malloc(sizeof(Logger));
    // setPeerAddr(log, addr);

    write(sock, "Hello", 5);
    break;
  }
}
