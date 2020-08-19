#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int fd;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  close(fd);

  return 0;
}
