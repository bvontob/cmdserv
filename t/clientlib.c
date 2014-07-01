#include "clientlib.h"

#include <err.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define HOSTBUFLEN  256
#define PORTBUFLEN  16
#define DEFAULTPORT "12346"

#define info(...) warnx(__VA_ARGS__)

int cmdserv_connect(int argc, char** argv) {
  struct addrinfo* ai;
  int ai_res;
  int sock_fd = -1;
  char host[HOSTBUFLEN], port[PORTBUFLEN];

  if (argc > 3)
    errx(EXIT_FAILURE, "Usage: %s [[HOST] PORT]", argc > 0 ? argv[0] : "BIN");

  if ((ai_res = getaddrinfo((argc > 2 ? argv[1] :
                             NULL),
                            (argc > 2 ? argv[2] :
                             argc > 1 ? argv[1] :
                             DEFAULTPORT),
                            &(struct addrinfo){
                              .ai_family   = AF_UNSPEC,
                              .ai_socktype = SOCK_STREAM,
                              .ai_flags    = AI_V4MAPPED
                            },
                            &ai)) != 0)
    errx(EXIT_FAILURE, "getaddrinfo(): %s", gai_strerror(ai_res));

  for (struct addrinfo* serv = ai;
       serv != NULL && sock_fd == -1;
       serv = serv->ai_next) {

    if ((ai_res = getnameinfo(serv->ai_addr,
                              serv->ai_addrlen,
                              host, sizeof(host),
                              port, sizeof(port),
                              NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
      errx(EXIT_FAILURE, "getaddrinfo(): %s", gai_strerror(ai_res));
    info("Trying %s port %s...", host, port);
    
    if ((sock_fd = socket(serv->ai_family,
                          serv->ai_socktype,
                          serv->ai_protocol)) == -1) {
      warn("socket()");
      continue;
    }
    
    if (connect(sock_fd, serv->ai_addr, serv->ai_addrlen) != 0) {
      warn("connect()");
      close(sock_fd);
      sock_fd = -1;
      continue;
    }
  }

  freeaddrinfo(ai);

  if (sock_fd == -1)
    errx(EXIT_FAILURE, "Connection failed.");
  info("Connected.");

  return sock_fd;
}
