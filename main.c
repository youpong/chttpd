#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

int main(int argc, char **argv) {
  struct sockaddr_in addr;
  int sv_sock;

  /* create a socket, endpoint of connection */
  sv_sock = socket(AF_INET, SOCK_STREAM, 0);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(12345);
  addr.sin_addr.s_addr = INADDR_ANY;
  bind(sv_sock, (struct sockaddr *)&addr, sizeof(addr));
  
  close(sv_sock);
  
  return 0;
}
