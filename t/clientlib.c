#include "clientlib.h"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define HOSTBUFLEN  256
#define PORTBUFLEN  16
#define DEFAULTPORT "12346"
#define RELAYBUFLEN 4096

int cmdserv_connect(int argc, char** argv) {
  struct addrinfo* ai;
  int ai_res;
  int fd = -1;
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
       serv != NULL && fd == -1;
       serv = serv->ai_next) {

    if ((ai_res = getnameinfo(serv->ai_addr,
                              serv->ai_addrlen,
                              host, sizeof(host),
                              port, sizeof(port),
                              NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
      errx(EXIT_FAILURE, "getaddrinfo(): %s", gai_strerror(ai_res));
    info("Trying %s port %s...", host, port);
    
    if ((fd = socket(serv->ai_family,
                          serv->ai_socktype,
                          serv->ai_protocol)) == -1) {
      warn("socket()");
      continue;
    }
    
    if (connect(fd, serv->ai_addr, serv->ai_addrlen) != 0) {
      warn("connect()");
      close(fd);
      fd = -1;
      continue;
    }
  }

  freeaddrinfo(ai);

  if (fd == -1)
    errx(EXIT_FAILURE, "Connection failed.");
  info("Connected.");

  return fd;
}

void cmdserv_relay(int in_fd, int out_fd) {
  ssize_t buflen, crlflen, writelen;
  char buf[RELAYBUFLEN];
  char crlf[RELAYBUFLEN * 2];

  while ((buflen = read(in_fd, buf, RELAYBUFLEN)) > 0) {
    crlflen = 0;
    for (ssize_t i = 0; i < buflen; i++) {
      if (buf[i] == '\n')
        crlf[crlflen++] = '\r';
      crlf[crlflen++] = buf[i];
    }
    
    if ((writelen = write(out_fd, crlf, crlflen)) == -1)
      err(EXIT_FAILURE, "write(#%d)", out_fd);

    if (writelen != crlflen) /* only here in test cases */
      errx(EXIT_FAILURE, "write(#%d): Could not write entire block", out_fd);
  }

  if (buflen == -1)
    err(EXIT_FAILURE, "read(#%d)", in_fd);
}

void cmdserv_close(int fd) {
  if (close(fd) != 0)
    err(EXIT_FAILURE, "close(#%d)", fd);
  info("Closed.");
}
