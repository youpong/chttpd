#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct {
  bool debug;
} Option;

Option *parse(int, char **);
void server_start(Option *);

int main(int argc, char **argv) {
  Option *opts = parse(argc, argv);
  server_start(opts);

  return 0;
}

Option *parse(int argvc, char **argv) {
  return NULL;
}

void server_start(Option *opts) {
}

void foo() {
  /* create a socket, endpoint of connection */
  int sv_sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(12345);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sv_sock, (struct sockaddr *)&addr, sizeof(addr));

  listen(sv_sock, 5);

  struct sockaddr_in client;
  socklen_t len = sizeof(client);
  int sock = accept(sv_sock, (struct sockaddr *)&client, &len);

  write(sock, "Hello", 5);

  close(sock);
  close(sv_sock);
}
