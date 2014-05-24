#define _GNU_SOURCE /* for asprintf() in stdio.h */
#define  DARWIN_C_LEVEL /* for asprintf() in stdio.h */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cmdserv_connection.h"
#include "cmdserv_connection_config.h"
#include "cmdserv_tokenize.h"


/**
 * Special internal states of the connection object.
 *
 * These states exist to deal with only one particular situation: If a
 * connection is closed explicitly from within a command handler.
 *
 * Explanation: Our cmdserv_connection_close() method closes the
 * connection and also destructs and frees the object ("ourselves").
 * The original creator will be informed through a callback (so it can
 * erase all references to the object and do its own housekeeping) --
 * so this is taken care of.  But if the method is called from within
 * a command handler, we can't really close the connection right away
 * and destroy us, as the command handler will return back to us (and
 * bad things will happen if we have destroyed ourselves already at
 * that point).
 *
 * To keep the external interface simple (the same method to close and
 * destruct the connection object can be used from everywhere), the
 * method has to know if it is being called from within a command
 * handler (the state will be set to CMDSERV_CONNECTION_STATE_HANDLED
 * before entering it).  In that case it should just note the close
 * request (setting the state to CMDSERV_CONNECTION_STATE_CLOSED), but
 * not take any real action on it.  Only once the command handler
 * returned to us, we're going to call the method once again.
 *
 * @see cmdserv_connection_close() cmdserv_connection_read()
 *      cmdserv_connection.state
 */
enum cmdserv_state {
  CMDSERV_CONNECTION_STATE_DEFAULT = 0, /**< No notable state         */
  CMDSERV_CONNECTION_STATE_HANDLED = 1, /**< In the command handler   */
  CMDSERV_CONNECTION_STATE_CLOSED  = 2, /**< Delayed close pending    */
};


/**
 * The cmdserv connection object.
 */
struct cmdserv_connection {
  unsigned long long int id;      /**< unique connection id           */

  int fd;                         /**< file descriptor                */

  struct sockaddr_in6 clientaddr; /**< client IP address/port         */
  socklen_t clientaddrlen;        /**< size of client IP address/port */
  char clienthost[256];           /**< client address as string       */
  char clientport[128];           /**< client port as string          */

  char buf[READBUF_SIZE];         /**< data read buffer               */
  size_t buflen;                  /**< current length of read buffer  */
  bool overflow;                  /**< true if buffer was overflowed  */

  char *argv[ARGC_MAX + 1];       /**< parsed command arguments       */

  enum cmdserv_state state;       /**< special object states          */

  enum cmdserv_lineterm lineterm; /**< setting for line termination   */
  bool rawmode;                   /**< skip the tokenizer             */

  void (*cmd_handler)(void *cmd_object,
                      cmdserv_connection* connection,
                      int argc,
                      char **argv);
  void *cmd_object;

  void (*open_handler)(void *open_object,
                       cmdserv_connection* connection);
  void *open_object;

  void (*close_handler)(void *close_object,
                        cmdserv_connection* connection);
  void *close_object;

  void (*log_handler)(void *log_object,
                      enum cmdserv_logseverity severity,
                      const char *msg);
  void *log_object;
};

static void cmdserv_connection_handle_line(cmdserv_connection* self);

int cmdserv_connection_fd(cmdserv_connection* self) {
  return self->fd;
}

unsigned long long int cmdserv_connection_id(cmdserv_connection* self) {
  return self->id;
}

char *cmdserv_connection_client(cmdserv_connection* self) {
  char *out = NULL;
  if (asprintf(&out, "[%s]:%s", self->clienthost, self->clientport) == -1)
    return NULL;
  return out;
}

void cmdserv_connection_vlog(cmdserv_connection* self,
                             enum cmdserv_logseverity severity,
                             const char *fmt, va_list ap) {
  if (self->log_handler) {
    char *msg  = NULL;
    char *cfmt = NULL;
    if (asprintf(&cfmt, "#%llu %s", self->id, fmt) >= 0) {
      if (vasprintf(&msg, cfmt, ap) >= 0) {
        self->log_handler(self->log_object, severity, msg);
        free(msg);
      }
      free(cfmt);
    }
  }
}

void __attribute__ ((format (printf, 3, 4)))
cmdserv_connection_log(cmdserv_connection* self,
                       enum cmdserv_logseverity severity,
                       const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  cmdserv_connection_vlog(self, severity, fmt, args);
  va_end(args);
}


void __attribute__ ((format (printf, 3, 4)))
cmdserv_connection_send_status(cmdserv_connection* self,
                               int status,
                               const char *fmt, ...) {
  va_list args;

  if (status < 100 || status > 999)
    status = 500;

  dprintf(self->fd, "%03d ", status);
  va_start(args, fmt);
  vdprintf(self->fd, fmt, args);
  va_end(args);
  dprintf(self->fd, "\r\n");
}

ssize_t cmdserv_connection_write(cmdserv_connection* self,
                                 const void *buf,
                                 size_t nbyte) {
  return write(self->fd, buf, nbyte);
}

void cmdserv_connection_print(cmdserv_connection* self,
                              const char *str) {
  dprintf(self->fd, "%s", str);
}

void cmdserv_connection_println(cmdserv_connection* self,
                                const char *str) {
  dprintf(self->fd, "%s\r\n", str);
}

void __attribute__ ((format (printf, 2, 3)))
cmdserv_connection_printf(cmdserv_connection* self,
                          const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vdprintf(self->fd, fmt, args);
  va_end(args);
}

void cmdserv_connection_close(cmdserv_connection* self) {
  if (self->state == CMDSERV_CONNECTION_STATE_HANDLED) {
    /* Take note for a delayed close if we're being handled right now */
    self->state = CMDSERV_CONNECTION_STATE_CLOSED;
    return;
  }

  cmdserv_connection_log(self, CMDSERV_INFO, "closing");

  if (self->close_handler)
    self->close_handler(self->close_object, self);
  close(self->fd);

  /* Be paranoid and zero out some values before freeing. */
  *self = (struct cmdserv_connection){
    .fd       = -1,
    .id       = 0,
    .buflen   = 0,
    .overflow = false
  };

  free(self);
}

void cmdserv_connection_read(cmdserv_connection* self) {
  size_t oldbuflen = self->buflen;

  ssize_t received = recv(self->fd,
                          self->buf + self->buflen,
                          READBUF_SIZE - self->buflen,
                          0);

  if (received == 0) {
    cmdserv_connection_log(self, CMDSERV_INFO, "client disconnect");
    cmdserv_connection_close(self);
    return;

  } else if (received == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return;
    cmdserv_connection_log(self, CMDSERV_ERR,
                           "recv() error: %s", strerror(errno));
    cmdserv_connection_close(self);
    return;
  }

  self->buflen += received;

  for (size_t i = oldbuflen; i < self->buflen; i++) {
    if (self->buf[i] == '\n'
        && (self->lineterm == CMDSERV_LINETERM_LF
            || self->lineterm == CMDSERV_LINETERM_LF_OR_CRLF
            || (self->lineterm == CMDSERV_LINETERM_CRLF
                && i > 0 && self->buf[i - 1] == '\r'))) {

      /* End of line found: Parse it -- unless overflow */
      if (self->overflow) {
        cmdserv_connection_send_status(self, 400, "Command too long");
        self->overflow = false;
      } else {
        self->buf[i] = '\0';
        if (self->lineterm == CMDSERV_LINETERM_CRLF
            || (self->lineterm == CMDSERV_LINETERM_LF_OR_CRLF
                && i > 0 && self->buf[i - 1] == '\r'))
          self->buf[i - 1] = '\0';

        self->state = CMDSERV_CONNECTION_STATE_HANDLED;
        cmdserv_connection_handle_line(self);
        if (self->state == CMDSERV_CONNECTION_STATE_CLOSED) {
          cmdserv_connection_close(self);
          return;
        }
        self->state = CMDSERV_CONNECTION_STATE_DEFAULT;
    
      }

      /* Move rest of buffer to beginning */
      self->buflen -= i + 1;
      memmove(self->buf,
              self->buf + i + 1,
              self->buflen);
      break;
    }
  }

  if (self->buflen == READBUF_SIZE) {
    self->overflow = true;
    self->buflen   = 0;
    cmdserv_connection_log(self, CMDSERV_WARNING, "command too long");
  }
}

static void cmdserv_connection_handle_line(cmdserv_connection *self) {
  int argc = 0;

  if (self->rawmode) {
    self->argv[0] = self->buf;
    self->argv[1] = NULL;
    argc = 1;
  } else {
    argc = cmdserv_tokenize(self->buf, self->argv, ARGC_MAX + 1);
  }

  if (argc == -1) {
    cmdserv_connection_log(self, CMDSERV_WARNING,
                           "too many arguments in command");
    cmdserv_connection_send_status(self, 400, "Too many arguments");
  } else {
    if (self->cmd_handler)
      self->cmd_handler(self->cmd_object, self, argc, self->argv);
  }
}

cmdserv_connection
*cmdserv_connection_create(int listener_fd,
                           unsigned long long int conn_id,
                           struct cmdserv_connection_config* config) {
  cmdserv_connection* self;

  if ((self = malloc(sizeof(struct cmdserv_connection))) == NULL)
    return NULL;

  *self = (struct cmdserv_connection){
    .id            = conn_id,
    .clientaddrlen = sizeof(((struct cmdserv_connection *)NULL)->clientaddr),
    .clienthost    = { '\0' },
    .clientport    = { '\0' },
    .buflen        = 0,
    .overflow      = false,
    .argv          = { NULL },
    .state         = CMDSERV_CONNECTION_STATE_DEFAULT,
    .lineterm      = CMDSERV_LINETERM_LF,
    .rawmode       = false,
    .cmd_handler   = config->cmd_handler,
    .cmd_object    = config->cmd_object,
    .open_handler  = config->open_handler,
    .open_object   = config->open_object,
    .close_handler = config->close_handler,
    .close_object  = config->close_object,
    .log_handler   = config->log_handler,
    .log_object    = config->log_object
  };

  self->fd = accept(listener_fd,
                    (struct sockaddr *)&self->clientaddr,
                    &self->clientaddrlen);

  if (self->fd == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      goto CMDSERV_CONNECTION_ABORT;
    cmdserv_connection_log(self, CMDSERV_ERR,
                           "accept() error: %s", strerror(errno));
    goto CMDSERV_CONNECTION_ABORT;
  }

  assert(self->clientaddrlen
         <= sizeof(((struct cmdserv_connection *)NULL)->clientaddr));

  {
    int gni_status
      = getnameinfo((struct sockaddr *)&self->clientaddr,
                    self->clientaddrlen,
                    self->clienthost,
                    sizeof(self->clienthost),
                    self->clientport,
                    sizeof(self->clientport),
                    NI_NUMERICHOST | NI_NUMERICSERV);
    if (gni_status != 0) {
      strcpy(self->clienthost, "?");
      strcpy(self->clientport, "?");
      cmdserv_connection_log(self, CMDSERV_ERR,
                             "getnameinfo(): %s", gai_strerror(gni_status));
    }
  }

  /*
   * We explicitly do set the newly accepted socket to O_NONBLOCK as
   * well due to the following notes.
   *
   * From the BUGS section of select(2) on Linux:
   *
   *     Under Linux, select() may report a socket file descriptor as
   *     "ready for reading", while nevertheless a subsequent read
   *     blocks.  This could for example happen when data has arrived
   *     but upon examination has wrong checksum and is discarded.
   *     There may be other circumstances in which a file descriptor
   *     is spuriously reported as ready.  Thus it may be safer to use
   *     O_NONBLOCK on sockets that should not block.
   *
   * From the CONFORMING TO section of accept(2) on Linux:
   *
   *     On Linux, the new socket returned by accept() does not
   *     inherit file status flags such as O_NONBLOCK and O_ASYNC from
   *     the listening socket.  This behavior differs from the
   *     canonical BSD sockets implementation.  Portable programs
   *     should not rely on inheritance or noninheritance of file
   *     status flags and always explicitly set all required flags on
   *     the socket returned from accept().
   */
  {
    int fdflags = fcntl(self->fd, F_GETFL, 0);
    if (fdflags == -1
        || fcntl(self->fd, F_SETFL, fdflags | O_NONBLOCK) == -1) {
      cmdserv_connection_log(self, CMDSERV_ERR,
                             "fcntl() error: %s", strerror(errno));
      close(self->fd);
      goto CMDSERV_CONNECTION_ABORT;
    }
  }

  if (self->open_handler)
    self->open_handler(self->open_object, self);

  return self;

 CMDSERV_CONNECTION_ABORT:
  free(self);
  return NULL;
}
