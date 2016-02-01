/* for asprintf() in stdio.h */
#define _GNU_SOURCE 
#define _DARWIN_C_SOURCE
#define _BSD_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cmdserv.h"
#include "cmdserv_helpers.h"

struct cmdserv {
  int connections_max;
  fd_set fds;                      /**< socket file descriptor list        */
  int    listener;                 /**< listening socket file descriptor   */
  int    fdmax;                    /**< maximum file descriptor number     */
  unsigned long long int conns;    /**< number of connections handled      */
  struct cmdserv_connection_config connection_config;

  time_t time_start;

  void (*log_handler)(void *log_object,
                      enum cmdserv_logseverity severity,
                      const char *msg);
  void *log_object;

  void (*close_handler_orig)(void *close_object,
                             cmdserv_connection* connection,
                             enum cmdserv_close_reason reason);
  void *close_object_orig;

  struct cmdserv_connection* conn[];
};


static void cmdserv_accept(cmdserv* self);
static int cmdserv_get_free_slot(cmdserv* self);
static int cmdserv_get_slot_id_from_fd(cmdserv* self, int fd);

void cmdserv_close_handler(void *close_object,
                           cmdserv_connection *connection,
                           enum cmdserv_close_reason reason);

void __attribute__ ((format (printf, 3, 0)))
cmdserv_vlog(cmdserv* self, enum cmdserv_logseverity severity,
             const char *fmt, va_list ap) {
  if (self->log_handler) {
    char *msg = NULL;
    if (vasprintf(&msg, fmt, ap) >= 0) {
      self->log_handler(self->log_object, severity, msg);
      free(msg);
    }
  }
}

void __attribute__ ((format (printf, 3, 4)))
cmdserv_log(cmdserv* self, enum cmdserv_logseverity severity,
            const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  cmdserv_vlog(self, severity, fmt, args);
  va_end(args);
}

char *cmdserv_server_status(cmdserv* self,
                            const char* lt,
                            unsigned long long int mark_conn) {
  char *idle = NULL;
  char *str = strdup("");
  char *tmp; /* Keep pointer around for free() */

  if (str == NULL)
    return NULL;

  tmp = str;
  if (asprintf(&str,
               "=======================================================================================%s"
               "SERVER STATUS%s"
               "=======================================================================================%s"
               "server uptime:       %s%s"
               "connections handled: %llu%s"
               "connections/sec:     %.2f%s"
               "listener fd:         #%d%s"
               "=======================================================================================%s"
               "slot  connection fd    connected     idle          client%s"
               "===== ========== ===== ============= ============= ====================================%s",
               lt, lt, lt,
               cmdserv_duration_str(self->time_start, time(NULL)), lt,
               self->conns, lt,
               (double)self->conns / (double)(time(NULL) + 1 - self->time_start), lt,
               self->listener, lt,
               lt, lt, lt)
      == -1) {
    free(tmp);
    return NULL;
  }
  free(tmp);

  for (int slot_id = 0; slot_id < self->connections_max; slot_id++) {
    if (self->conn[slot_id] == NULL)
      continue;

    idle = strdup(cmdserv_duration_str(0,
        cmdserv_connection_time_idle(self->conn[slot_id])));
    tmp = str;
    if (asprintf(&str,
                 "%s%s% 4d #%-9llu #%-4d %13s %13s %s%s",
                 str,
                 (cmdserv_connection_id(self->conn[slot_id]) == mark_conn
                  ? "*" : " "),
                 slot_id + 1,
                 cmdserv_connection_id(self->conn[slot_id]),
                 cmdserv_connection_fd(self->conn[slot_id]),
                 cmdserv_duration_str(0, cmdserv_connection_time_connected(self->conn[slot_id])),
                 idle,
                 cmdserv_connection_client(self->conn[slot_id]),
                 lt)
        == -1) {
      free(tmp);
      return NULL;
    }
    free(idle);
    free(tmp);
  }

  return str;
}


void cmdserv_shutdown(cmdserv* self) {
  if (self == NULL)
    return;

  cmdserv_log(self, CMDSERV_INFO, "server shutdown initialized");

  for (int slot_id = 0; slot_id < self->connections_max; slot_id++) {
    if (self->conn[slot_id] != NULL) {
      cmdserv_connection_close(self->conn[slot_id],
                               CMDSERV_SERVER_SHUTDOWN);
      self->conn[slot_id] = NULL;
    }
  }

  cmdserv_log(self, CMDSERV_INFO, "server shutdown reached");
  free(self);
}


cmdserv* cmdserv_start(struct cmdserv_config config) {
  cmdserv* self;
  int saverrno = 0;

  struct sockaddr_in6 servaddr = {
    .sin6_family = AF_INET6,
    .sin6_addr   = IN6ADDR_ANY_INIT,
    .sin6_port   = htons(config.port)
  };

  if ((self = malloc(sizeof(struct cmdserv)
                     + (sizeof(cmdserv_connection*)
                        * config.connections_max)))
      == NULL) {
    saverrno = errno;
    goto CMDSERV_ABORT;
  }

  *self = (struct cmdserv){
    .listener          = -1,
    .fdmax             = 0,
    .conns             = 0,
    .time_start        = time(NULL),
    .log_handler       = config.log_handler,
    .log_object        = config.log_object,
    .connections_max   = config.connections_max,
    .connection_config = config.connection_config
  };

  /*
   * We need to have access to the close_handler ourselves for
   * clean-up of terminated connections: So we overwrite the
   * close_handler and the close_object provided by our caller with
   * our own -- but only after we kept a copy of them both that we'll
   * use to chain to from our own handler.
   */
  self->close_handler_orig = self->connection_config.close_handler;
  self->close_object_orig  = self->connection_config.close_object;
  self->connection_config.close_handler = &cmdserv_close_handler;
  self->connection_config.close_object  = self;

  for (int slot_id = 0; slot_id < self->connections_max; slot_id++)
    self->conn[slot_id] = NULL;
  FD_ZERO(&self->fds);

  if ((self->listener = socket(AF_INET6, SOCK_STREAM, 0)) == -1) {
    saverrno = errno;
    cmdserv_log(self, CMDSERV_ERR, "socket() error: %s", strerror(saverrno));
    goto CMDSERV_ABORT;
  }

  FD_SET(self->listener, &self->fds);
  self->fdmax = self->listener;

  if (setsockopt(self->listener, SOL_SOCKET,
                 SO_REUSEADDR, &(int){1}, sizeof(int))
      == -1) {
    saverrno = errno;
    cmdserv_log(self, CMDSERV_ERR, "setsockopt() error: %s", strerror(saverrno));
    goto CMDSERV_ABORT;
  }

  /*
   * Setting O_NONBLOCK on the listener. From the NOTES section of
   * accept(2) on Linux:
   *
   *     There may not always be a connection waiting after a SIGIO is
   *     delivered or select(2) or poll(2) return a readability event
   *     because the connection might have been removed by an
   *     asynchronous network error or another thread before accept()
   *     is called.  If this happens then the call will block waiting
   *     for the next connection to arrive.  To ensure that accept()
   *     never blocks, the passed socket sockfd needs to have the
   *     O_NONBLOCK flag set (see socket(7)).
   */
  {
    int fdflags = fcntl(self->listener, F_GETFL, 0);
    if (fdflags == -1
        || fcntl(self->listener, F_SETFL, fdflags | O_NONBLOCK) == -1) {
      saverrno = errno;
      cmdserv_log(self, CMDSERV_ERR, "fcntl() error: %s", strerror(saverrno));
      goto CMDSERV_ABORT;
    }
  }

  if (bind(self->listener,
           (struct sockaddr *)&servaddr, sizeof(servaddr))
      == -1) {
    saverrno = errno;
    cmdserv_log(self, CMDSERV_ERR, "bind() error: %s", strerror(saverrno));
    goto CMDSERV_ABORT;
  }

  if (listen(self->listener, config.connections_backlog)
      == -1) {
    saverrno = errno;
    cmdserv_log(self, CMDSERV_ERR, "listen() error: %s", strerror(saverrno));
    goto CMDSERV_ABORT;
  }

  cmdserv_log(self, CMDSERV_INFO,
              "server ready for connections on port %u",
              config.port);

  return self;

 CMDSERV_ABORT:
  cmdserv_shutdown(self);
  errno = saverrno;
  return NULL;
}


void cmdserv_close_handler(void *object,
                           cmdserv_connection* connection,
                           enum cmdserv_close_reason reason) {
  cmdserv *self = object;
  int fd = cmdserv_connection_fd(connection);

  /* Chain through to the close handler our caller originally requested */
  if (self->close_handler_orig)
    self->close_handler_orig(self->close_object_orig, connection, reason);

  /*
   * Remove connection, but skip for those that have never been added
   * to a slot/the FD list (e.g. on too many connections)
   */
  if (FD_ISSET(fd, &self->fds)) {
    FD_CLR(fd, &self->fds);
    self->conn[cmdserv_get_slot_id_from_fd(self, fd)] = NULL;
  }
}


/**
 * Private method to handle a new incoming connection.
 */
static void cmdserv_accept(cmdserv* self) {
  int slot_id;
  struct cmdserv_connection* new_conn;

  self->conns++;
  slot_id = cmdserv_get_free_slot(self);

  if ((new_conn = cmdserv_connection_create(self->listener,
                                            self->conns,
                                            &self->connection_config,
                                            slot_id == -1
                                            ? CMDSERV_SERVER_TOO_MANY_CONNECTIONS
                                            : CMDSERV_NO_CLOSE))
      == NULL) {
    cmdserv_log(self, CMDSERV_ERR,
                "cmdserv_connection_create(#%llu) failed: %s",
                self->conns,
                strerror(errno));
    return;
  }

  if (slot_id == -1) {
    cmdserv_log(self, CMDSERV_WARNING,
                "too many connections, turning #%llu away",
                self->conns);
    cmdserv_connection_close(new_conn, CMDSERV_SERVER_TOO_MANY_CONNECTIONS);
    return;
  }

  self->conn[slot_id] = new_conn;
  FD_SET(cmdserv_connection_fd(self->conn[slot_id]), &self->fds);

  if (cmdserv_connection_fd(self->conn[slot_id]) > self->fdmax)
    self->fdmax = cmdserv_connection_fd(self->conn[slot_id]);
}

static int cmdserv_get_slot_id_from_fd(cmdserv* self, int fd) {
  for (int slot_id = 0; slot_id < self->connections_max; slot_id++)
    if (self->conn[slot_id] != NULL
        && cmdserv_connection_fd(self->conn[slot_id]) == fd) 
      return slot_id;

  cmdserv_log(self, CMDSERV_WARNING,
              "cmdserv_get_slot_id_from_fd(#%d): "
              "FD not associated with any connection",
              fd);
  return -1;
}

static int cmdserv_get_free_slot(cmdserv* self) {
  for (int slot_id = 0; slot_id < self->connections_max; slot_id++)
    if (self->conn[slot_id] == NULL) 
      return slot_id;
  return -1;
}


void cmdserv_sleep(cmdserv* self, struct timeval *timeout) {
  /* Some implementations of select() change the timeout parameter to
     reflect the time actually slept. So we need a copy. Also the file
     descriptor sets can become undefined on errors in select(), so
     also a copy there... */
  struct timeval timeout_copy = *timeout;
  fd_set read_fds;

  FD_ZERO(&read_fds);
  FD_SET(self->listener, &read_fds);

  for (int slot_id = 0; slot_id < self->connections_max; slot_id++) {
    if (self->conn[slot_id] != NULL) {
      time_t client_timeout = cmdserv_connection_client_timeout(self->conn[slot_id]);
      if (client_timeout > 0
          && cmdserv_connection_time_idle(self->conn[slot_id]) > client_timeout) {
        cmdserv_connection_log(self->conn[slot_id], CMDSERV_INFO, "client timeout");
        cmdserv_connection_close(self->conn[slot_id], CMDSERV_CLIENT_TIMEOUT);
      } else {
        FD_SET(cmdserv_connection_fd(self->conn[slot_id]), &read_fds);
      }
    }
  }

  if (select(self->fdmax + 1, &read_fds, NULL, NULL, &timeout_copy)
      == -1) {
    if (errno == EINTR)
      cmdserv_log(self, CMDSERV_DEBUG, "select() interrupted by signal");
    else
      cmdserv_log(self, CMDSERV_ERR, "select() error: %s", strerror(errno));
    return;
  }

  for (int fd = 0; fd <= self->fdmax; fd++) {
    if (FD_ISSET(fd, &read_fds)) {
      if (fd == self->listener)
        cmdserv_accept(self);
      else
        cmdserv_connection_read(self->conn[cmdserv_get_slot_id_from_fd(self, fd)]);
    }
  }
}
