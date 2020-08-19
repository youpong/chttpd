#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int sv_sock;

  /* create a socket, endpoint of connection */
  sv_sock = socket(AF_INET, SOCK_STREAM, 0);

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

  return 0;
}
