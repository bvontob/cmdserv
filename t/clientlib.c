#include "clientlib.h"

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define HOSTBUFLEN  256
#define PORTBUFLEN  16
#define DEFAULTPORT "12346"
#define RELAYBUFLEN 4096

int src_port = 0;

int cmdserv_connect(int argc, char** argv) {
  struct addrinfo* ai;
  char *host = NULL;
  const char *port = DEFAULTPORT;
  int ai_res;
  int fd = -1;
  char realhost[HOSTBUFLEN], realport[PORTBUFLEN];

  if (argc > 4) {
    errx(EXIT_FAILURE,
         "Usage: %s [[[SRC_PORT] HOST] PORT]",
         argv[0]);
  } else if (argc > 3) {
    if (src_port == 0)
      src_port = atoi(argv[1]);
    host     = argv[2];
    port     = argv[3];
  } else if (argc > 2) {
    host     = argv[1];
    port     = argv[2];
  } else if (argc > 1) {
    port     = argv[1];
  }

  if ((ai_res = getaddrinfo(host,
                            port,
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
                              realhost, HOSTBUFLEN,
                              realport, PORTBUFLEN,
                              NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
      errx(EXIT_FAILURE, "getaddrinfo(): %s", gai_strerror(ai_res));

    if (src_port > 0)
      info("Trying %s destination port %s source port %d...",
           realhost, realport, src_port);
    else
      info("Trying %s port %s...", realhost, realport);
    
    if ((fd = socket(serv->ai_family,
                     serv->ai_socktype,
                     serv->ai_protocol)) == -1) {
      warn("socket()");
      continue;
    }
    
    if (src_port > 0) {
      if (setsockopt(fd, SOL_SOCKET,
                     SO_REUSEADDR, &(int){1}, sizeof(int))
          == -1) {
        warn("setsockopt(#%d, ..., SO_REUSEADDR, ...)", fd);
        close(fd);
        fd = -1;
        continue;
      }


      if ((serv->ai_family == AF_INET6
           ? bind(fd,
                  (struct sockaddr*)&(struct sockaddr_in6){
                    .sin6_family      = AF_INET6,
                    .sin6_addr        = IN6ADDR_ANY_INIT,
                    .sin6_port        = htons(src_port)
                  },
                  sizeof(struct sockaddr_in6))
           : bind(fd,
                  (struct sockaddr*)&(struct sockaddr_in){
                    .sin_family      = AF_INET,
                    .sin_addr.s_addr = INADDR_ANY,
                    .sin_port        = htons(src_port)
                  },
                  sizeof(struct sockaddr_in)))
          != 0) {
        warn("bind(#%d, ...)", fd);
        close(fd);
        fd = -1;
        continue;
      }
    }

    if (connect(fd, serv->ai_addr, serv->ai_addrlen) != 0) {
      warn("connect(#%d, ...)", fd);
      close(fd);
      fd = -1;
      continue;
    }
  }

  freeaddrinfo(ai);

  if (src_port > 0)
    src_port++;

  if (fd == -1)
    errx(EXIT_FAILURE, "connection failed");
  info("#%d connected", fd);

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
  info("#%d closed", fd);
}

int millisleep(unsigned int ms) {
  struct timespec in, rest;
  int res;

  rest = (struct timespec) {
    .tv_sec  = ms / 1000,
    .tv_nsec = (ms % 1000) * 1000 * 1000
  };

  do {
    in  = rest;
    res = nanosleep(&in, &rest);
  } while (res != 0 && errno == EINTR);

  return res;
}
