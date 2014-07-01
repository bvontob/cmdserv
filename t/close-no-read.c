#include "clientlib.h"

int main(int argc, char** argv) {
  int fd = cmdserv_connect(argc, argv);
  cmdserv_relay(STDIN_FILENO, fd);
  cmdserv_close(fd);
}
