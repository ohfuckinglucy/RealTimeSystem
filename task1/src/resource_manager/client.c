#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define EXAMPLE_SOCK_PATH "/tmp/example_resmgr.sock"

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s <message>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, EXAMPLE_SOCK_PATH, sizeof(addr.sun_path) - 1);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("connect");
    close(fd);
    return EXIT_FAILURE;
  }

  const char *msg = argv[1];
  size_t len = strlen(msg);
  if (send(fd, msg, len, 0) != (ssize_t)len) {
    perror("send");
    close(fd);
    return EXIT_FAILURE;
  }

  char buf[1024];
  ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
  if (n < 0) {
    perror("recv");
    close(fd);
    return EXIT_FAILURE;
  }
  buf[n] = '\0';
  printf("response: %s\n", buf);

  close(fd);
  return EXIT_SUCCESS;
}
