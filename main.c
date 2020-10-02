#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
  bool debug;
} Option;

Option *parse(int, char **);
void server_start(Option *);
int sv_listen(Option *);
int sv_accept(int);

int main(int argc, char **argv) {
  Option *opts = parse(argc, argv);
  server_start(opts);

  return 0;
}

Option *parse(int argvc, char **argv) {
  return NULL;
}

void server_start(Option *opts) {

  int sv_sock = sv_listen(opts);

  while (true) {
    int sock = sv_accept(sv_sock);

    pid_t pid = fork();
    switch (pid) {
    case -1:
      // error
      break;
    case 0:
      // child
      write(sock, "Hello", 5);
      close(sock);
      _exit(0);
      break;
    default:
        // parent
        ;
    }
  }

  close(sv_sock);
}

int sv_listen(Option *opts) {
  /* create a socket, endpoint of connection */
  int sv_sock = socket(AF_INET, SOCK_STREAM, 0);

  /* bind */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(12345);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sv_sock, (struct sockaddr *)&addr, sizeof(addr));

  /* listen */
  listen(sv_sock, 5);

  return sv_sock;
}

int sv_accept(int sv_sock) {
  struct sockaddr_in client;
  socklen_t len = sizeof(client);
  int sock = accept(sv_sock, (struct sockaddr *)&client, &len);

  return sock;
}
