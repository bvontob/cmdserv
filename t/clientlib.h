#ifndef CLIENTLIB_H
#define CLIENTLIB_H

#include <err.h>
#include <stdlib.h>
#include <unistd.h>

#define info(...) warnx(__VA_ARGS__)

int cmdserv_connect(int argc, char** argv);

void cmdserv_relay(int in_fd, int out_fd);

void cmdserv_close(int fd);

#endif /* CLIENTLIB_H */
