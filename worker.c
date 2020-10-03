#include "httpd.h"
#include <unistd.h>

void worker_start(int sock, Option *opt) {
  write(sock, "Hello", 5);
}
