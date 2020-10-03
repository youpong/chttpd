#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  bool debug;
} Option;

typedef struct {
  char *peer_addr;
  time_t date;
} Logger;

Option *parse(int, char **);
void server_start(Option *);
int sv_listen(Option *);
int sv_accept(int);
void worker_start(int, Option *);

int main(int argc, char **argv) {
  Option *opt = parse(argc, argv);
  server_start(opt);

  return 0;
}

Option *parse(int argvc, char **argv) {
  return NULL;
}

void server_start(Option *opt) {

  int sv_sock = sv_listen(opt);

  while (true) {
    int sock = sv_accept(sv_sock);

    pid_t pid = fork();
    switch (pid) {
    case -1: // error
      perror("fork");
      exit(1);
    case 0: // child
      worker_start(sock, opt);
      close(sock);
      _exit(0);
    default: // parent
        ;
    }
  }

  close(sv_sock);
}

int sv_listen(Option *opt) {
  int sv_sock;

  /* create a socket, endpoint of connection */
  if ((sv_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  /* bind */
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8088);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(sv_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }

  /* listen */
  if (listen(sv_sock, 5) == -1) {
    perror("listen");
    exit(1);
  }

  return sv_sock;
}

int sv_accept(int sv_sock) {
  int sock;  
  struct sockaddr_in client;
  socklen_t len = sizeof(client);

  if ((sock = accept(sv_sock, (struct sockaddr *)&client, &len)) < 0) {
    perror("accept");
    exit(1);
  }

  return sock;
}

void worker_start(int sock, Option *opt) {
  write(sock, "Hello", 5);
}
