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

#include "intercept.h"
#include "cmdserv_helpers.h"
#include "cmdserv_connection.h"
#include "cmdserv_connection_config.h"


#ifdef MSG_NOSIGNAL
#define SETSOCKOPT_NOSIGPIPE_UNLESS_MSG_NOSIGNAL(FD) (0)
#else
/**
 * Suppress SIGPIPE using SO_NOSIGPIPE on the socket for systems that
 * do not have the MSG_NOSIGNAL flag on send().
 *
 * @param
 *
 *     The socket file descriptor.
 *
 * @return
 *
 *     0 on success, -1 on failure (in which case global errno will be
 *     set).
 */
#define SETSOCKOPT_NOSIGPIPE_UNLESS_MSG_NOSIGNAL(FD)                    \
  setsockopt(FD, SOL_SOCKET, SO_NOSIGPIPE, &(int){1}, sizeof(int))
#define MSG_NOSIGNAL 0
#endif


/**
 * Returns the current line termination string for the
 * cmdserv_connection object o.
 */
#define EOL(o) ((o)->lineterm == CMDSERV_LINETERM_LF ? "\n" : "\r\n")


/**
 * Macro for use with snprintf()/vsnprintf() and the internal
 * cmdserv_connection writebuf.
 *
 * The macro compares the current writebuf_size of cmdserv_connection
 * object o with the given string length l.  If the string fits,
 * nothing is done.  If the string would be/was truncated, the size of
 * the buffer is increased using cmdserv_connection_resize_writebuf()
 * and the macro jumps to label using goto.  If the reallocation
 * fails, the macro returns from the function it's used in with an
 * exit code of -1, errno will be set by
 * cmdserv_connection_resize_writebuf().
 *
 * If the given length l is zero or negative, the macro immediately
 * returns from the function it's used in with an exit code of l.
 */
#define WRITEBUF_CHECK_AND_RESIZE(o, l, a, label) do {                  \
    if ((a) == -1)                                                      \
      return (a);                                                       \
    (l) += (a);                                                         \
    if ((l) >= (o)->writebuf_size) {                                    \
      if (cmdserv_connection_resize_writebuf((o), (l) + 1) == NULL)     \
        return -1;                                                      \
      (l) = 0;                                                          \
      goto label;                                                       \
    }                                                                   \
  } while (0)


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
 * request (setting the close_reason), but not take any real action on
 * it.  Only once the command handler returned to us, we're going to
 * call the method once again.
 *
 * @see cmdserv_connection_close() cmdserv_connection_read()
 *      cmdserv_connection.state
 */
enum cmdserv_state {
  CMDSERV_CONNECTION_STATE_DEFAULT = 0, /**< No notable state         */
  CMDSERV_CONNECTION_STATE_HANDLED = 1, /**< In the command handler   */
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

  time_t time_connect;            /**< time of client connection      */
  time_t time_last;               /**< time of last client activity   */
  time_t client_timeout;          /**< inactivity timeout config      */

  ssize_t writebuf_size;          /**< maximum size of write buffer   */
  char *writebuf;                 /**< buffer for snprintf() strings  */

  size_t readbuf_size;            /**< maximum size of read buffer    */
  char *buf;                      /**< data read buffer               */
  size_t buflen;                  /**< current length of read buffer  */
  bool overflow;                  /**< true if buffer was overflowed  */

  unsigned int argc_max;          /**< size of argv (without NULL)    */
  char **argv;                    /**< parsed command arguments       */
  int argc;                       /**< number of parsed command args  */

  enum cmdserv_state state;       /**< special object states          */

  enum cmdserv_close_reason close_reason;

  enum cmdserv_lineterm lineterm; /**< setting for line termination   */

  cmdserv_tokenizer tokenizer;

  void (*cmd_handler)(void *cmd_object,
                      cmdserv_connection* connection,
                      int argc,
                      char **argv);
  void *cmd_object;

  void (*open_handler)(void *open_object,
                       cmdserv_connection* connection,
                       enum cmdserv_close_reason reason);
  void *open_object;

  void (*close_handler)(void *close_object,
                        cmdserv_connection* connection,
                        enum cmdserv_close_reason reason);
  void *close_object;

  void (*log_handler)(void *log_object,
                      enum cmdserv_logseverity severity,
                      const char *msg);
  void *log_object;
};

static void cmdserv_connection_handle_line(cmdserv_connection* self);
static void cmdserv_connection_free(cmdserv_connection* self);
static void *cmdserv_connection_resize_writebuf(cmdserv_connection* self,
                                                ssize_t size);

int cmdserv_connection_fd(cmdserv_connection* self) {
  return self->fd;
}

unsigned long long int cmdserv_connection_id(cmdserv_connection* self) {
  return self->id;
}

time_t cmdserv_connection_time_idle(cmdserv_connection* self) {
  return time(NULL) - self->time_last;
}

time_t cmdserv_connection_client_timeout(cmdserv_connection* self) {
  return self->client_timeout;
}

void cmdserv_connection_set_client_timeout(cmdserv_connection* self, time_t timeout) {
  self->client_timeout = timeout > 0 ? timeout : 0;
}

time_t cmdserv_connection_time_connected(cmdserv_connection* self) {
  return time(NULL) - self->time_connect;
}

cmdserv_tokenizer cmdserv_connection_tokenizer(cmdserv_connection* self,
                                               cmdserv_tokenizer tokenizer) {
  cmdserv_tokenizer old_tokenizer = self->tokenizer;
  self->tokenizer = tokenizer;
  return old_tokenizer;
}

char *cmdserv_connection_client(cmdserv_connection* self) {
  char *out = NULL;
  if (asprintf(&out, "[%s]:%s", self->clienthost, self->clientport) == -1)
    return NULL;
  return out;
}

char *cmdserv_connection_command_string(cmdserv_connection* self,
                                        enum cmdserv_string_treatment trtmt) {
  (void)trtmt;
  char *a = strdup("");
  char *b = NULL;
  int len;
  
  for (int i = 0; i < self->argc; i++) {
    if ((len = asprintf(&b, "%s\"%s\" ", a, self->argv[i])) == -1) {
      free(a);
      return NULL;
    }
    free(a);
    a = b;
    b = NULL;
  }

  /* Cut off additional space */
  if (len > 0)
    a[len - 1] = '\0';
  
  b = cmdserv_logsafe_str(a);
  free(a);
  return b;
}

void __attribute__ ((format (printf, 3, 0)))
cmdserv_connection_vlog(cmdserv_connection* self,
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


ssize_t __attribute__ ((format (printf, 3, 4)))
cmdserv_connection_send_status(cmdserv_connection* self,
                               int status,
                               const char *fmt, ...) {
  va_list args;
  ssize_t len = 0, added;

  if (status < 100 || status > 999)
    status = 500;

 CMDSERV_CONNECTION_SEND_STATUS_REDO:
  added = snprintf(self->writebuf + len,
                   self->writebuf_size - len,
                   "%03d ", status);
  WRITEBUF_CHECK_AND_RESIZE(self, len, added, CMDSERV_CONNECTION_SEND_STATUS_REDO);

  va_start(args, fmt);
  added = vsnprintf(self->writebuf + len,
                    self->writebuf_size - len,
                    fmt, args);
  va_end(args);
  WRITEBUF_CHECK_AND_RESIZE(self, len, added, CMDSERV_CONNECTION_SEND_STATUS_REDO);

  added = snprintf(self->writebuf + len,
                   self->writebuf_size - len,
                   "%s", EOL(self));
  WRITEBUF_CHECK_AND_RESIZE(self, len, added, CMDSERV_CONNECTION_SEND_STATUS_REDO);

  return cmdserv_connection_send(self, self->writebuf, len, MSG_NOSIGNAL);
}

ssize_t cmdserv_connection_send(cmdserv_connection* self,
                                const void *buf,
                                size_t nbyte,
                                int flags) {
  return send(self->fd, buf, nbyte, flags);
}

ssize_t cmdserv_connection_print(cmdserv_connection* self,
                                 const char *str) {
  return cmdserv_connection_send(self,
                                 str,
                                 strlen(str),
                                 MSG_NOSIGNAL);
}

ssize_t cmdserv_connection_println(cmdserv_connection* self,
                                   const char *str) {
  return cmdserv_connection_printf(self, "%s%s", str, EOL(self));
}

ssize_t __attribute__ ((format (printf, 2, 3)))
cmdserv_connection_printf(cmdserv_connection* self,
                          const char *fmt, ...) {
  ssize_t sent;
  va_list args;
  va_start(args, fmt);
  sent = cmdserv_connection_vprintf(self, fmt, args);
  va_end(args);
  return sent;
}

ssize_t __attribute__ ((format (printf, 2, 0)))
cmdserv_connection_vprintf(cmdserv_connection* self,
                           const char *fmt, va_list ap) {
  ssize_t len = 0, added;
  va_list ap2;

 CMDSERV_CONNECTION_VPRINTF_REDO:
  va_copy(ap2, ap);
  added = vsnprintf(self->writebuf, self->writebuf_size, fmt, ap2);
  va_end(ap2);
  WRITEBUF_CHECK_AND_RESIZE(self, len, added, CMDSERV_CONNECTION_VPRINTF_REDO);

  return cmdserv_connection_send(self, self->writebuf, len, MSG_NOSIGNAL);
}

void cmdserv_connection_close(cmdserv_connection* self,
                              enum cmdserv_close_reason reason) {
  self->close_reason = (reason == CMDSERV_NO_CLOSE
                        ? CMDSERV_APPLICATION_CLOSE
                        : reason);

  if (self->state == CMDSERV_CONNECTION_STATE_HANDLED)
    return;

  cmdserv_connection_log(self, CMDSERV_INFO, "closing");

  if (self->close_handler)
    self->close_handler(self->close_object, self, self->close_reason);

  cmdserv_connection_free(self);
}

void cmdserv_connection_read(cmdserv_connection* self) {
  size_t oldbuflen = self->buflen;

  ssize_t received = recv(self->fd,
                          self->buf + self->buflen,
                          self->readbuf_size - self->buflen,
                          0);

  if (received == 0) {
    cmdserv_connection_log(self, CMDSERV_INFO, "client disconnect");
    cmdserv_connection_close(self, CMDSERV_CLIENT_DISCONNECT);
    return;

  } else if (received == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return;
    cmdserv_connection_log(self, CMDSERV_ERR,
                           "recv() error: %s", strerror(errno));
    cmdserv_connection_close(self, CMDSERV_CLIENT_RECEIVE_ERROR);
    return;
  }

  self->time_last = time(NULL);

  self->buflen += received;

  for (size_t i = oldbuflen; i < self->buflen; i++) {
    if (self->buf[i] == '\n'
        && (self->lineterm == CMDSERV_LINETERM_LF
            || self->lineterm == CMDSERV_LINETERM_CRLF_OR_LF
            || (self->lineterm == CMDSERV_LINETERM_CRLF
                && i > 0 && self->buf[i - 1] == '\r'))) {

      /* End of line found: Parse it -- unless overflow */
      if (self->overflow) {
        cmdserv_connection_send_status(self, 400, "Command too long");
        self->overflow = false;
      } else {
        self->buf[i] = '\0';
        if (self->lineterm == CMDSERV_LINETERM_CRLF
            || (self->lineterm == CMDSERV_LINETERM_CRLF_OR_LF
                && i > 0 && self->buf[i - 1] == '\r'))
          self->buf[i - 1] = '\0';

        self->state = CMDSERV_CONNECTION_STATE_HANDLED;
        cmdserv_connection_handle_line(self);
        self->state = CMDSERV_CONNECTION_STATE_DEFAULT;

        if (self->close_reason != CMDSERV_NO_CLOSE) {
          cmdserv_connection_close(self, self->close_reason);
          return;
        }
      }

      /* Move rest of buffer to beginning */
      self->buflen -= i + 1;
      memmove(self->buf,
              self->buf + i + 1,
              self->buflen);
      i = 0; /* Try again for one more line */
    }
  }

  if (self->buflen == self->readbuf_size) {
    self->overflow = true;
    self->buflen   = 0;
    cmdserv_connection_log(self, CMDSERV_WARNING, "command too long");
  }
}

static void cmdserv_connection_handle_line(cmdserv_connection *self) {
  if (self->tokenizer == NULL) {
    self->argv[0] = self->buf;
    self->argv[1] = NULL;
    self->argc = 1;
  } else {
    self->argc = self->tokenizer(self->buf, self->argv, self->argc_max + 1);
  }

  if (self->argc == -1) {
    cmdserv_connection_log(self, CMDSERV_WARNING,
                           "too many arguments in command");
    cmdserv_connection_send_status(self, 400, "Too many arguments");
  } else {
    if (self->cmd_handler)
      self->cmd_handler(self->cmd_object, self, self->argc, self->argv);
  }

  self->argc    = 0;
  self->argv[0] = NULL;
}

cmdserv_connection
*cmdserv_connection_create(int listener_fd,
                           unsigned long long int conn_id,
                           struct cmdserv_connection_config* config,
                           enum cmdserv_close_reason close_reason) {
  cmdserv_connection* self;
  int saverrno = 0;

  if ((self = malloc(sizeof(struct cmdserv_connection))) == NULL)
    return NULL;

  *self = (struct cmdserv_connection){
    .id            = conn_id,
    .clientaddrlen = sizeof(((struct cmdserv_connection *)NULL)->clientaddr),
    .clienthost    = { '\0' },
    .clientport    = { '\0' },
    .time_connect  = time(NULL),
    .time_last     = time(NULL),
    .client_timeout= config->client_timeout,
    .writebuf_size = 1024,
    .readbuf_size  = config->readbuf_size,
    .buflen        = 0,
    .overflow      = false,
    .argc_max      = config->argc_max,
    .state         = CMDSERV_CONNECTION_STATE_DEFAULT,
    .close_reason  = CMDSERV_NO_CLOSE,
    .lineterm      = CMDSERV_LINETERM_CRLF_OR_LF,
    .tokenizer     = config->tokenizer,
    .cmd_handler   = config->cmd_handler,
    .cmd_object    = config->cmd_object,
    .open_handler  = config->open_handler,
    .open_object   = config->open_object,
    .close_handler = config->close_handler,
    .close_object  = config->close_object,
    .log_handler   = config->log_handler,
    .log_object    = config->log_object
  };

  if ((self->argv = calloc(self->argc_max + 1, sizeof(char*))) == NULL) {
    saverrno = errno;
    goto CMDSERV_CONNECTION_ABORT;
  }
  self->argv[0] = NULL;

  if ((self->buf = calloc(self->readbuf_size, sizeof(char))) == NULL) {
    saverrno = errno;
    goto CMDSERV_CONNECTION_ABORT;
  }

  if ((self->writebuf = calloc(self->writebuf_size, sizeof(char))) == NULL) {
    saverrno = errno;
    goto CMDSERV_CONNECTION_ABORT;
  }

  self->fd = accept(listener_fd,
                    (struct sockaddr *)&self->clientaddr,
                    &self->clientaddrlen);

  if (self->fd == -1) {
    saverrno = errno;
    if (saverrno == EAGAIN || saverrno == EWOULDBLOCK || saverrno == EINTR)
      goto CMDSERV_CONNECTION_ABORT;
    cmdserv_connection_log(self, CMDSERV_ERR,
                           "accept() error: %s", strerror(saverrno));
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
      saverrno = errno;
      cmdserv_connection_log(self, CMDSERV_ERR,
                             "fcntl() error: %s", strerror(saverrno));
      goto CMDSERV_CONNECTION_ABORT;
    }
  }

  if (SETSOCKOPT_NOSIGPIPE_UNLESS_MSG_NOSIGNAL(self->fd)) {
    saverrno = errno;
    cmdserv_connection_log(self, CMDSERV_ERR,
                           "setsockopt() error: %s", strerror(saverrno));
    goto CMDSERV_CONNECTION_ABORT;
  }

  {
    char *client_info = cmdserv_connection_client(self);
    cmdserv_connection_log(self, CMDSERV_INFO,
                           "connected from %s",
                           client_info);
    free(client_info);
  }

  if (self->open_handler)
    self->open_handler(self->open_object, self, close_reason);

  return self;

 CMDSERV_CONNECTION_ABORT:
  cmdserv_connection_free(self);
  errno = saverrno;
  return NULL;
}

static void cmdserv_connection_free(cmdserv_connection* self) {
  if (self->fd != -1)
    close(self->fd);

  free(self->argv);
  free(self->buf);
  free(self->writebuf);

  /* Be paranoid and zero out before freeing. */
  *self = (struct cmdserv_connection){
    .fd       = -1,
    .overflow = false
  };

  free(self);
}

static void *cmdserv_connection_resize_writebuf(cmdserv_connection* self,
                                                ssize_t req_size) {
  ssize_t new_size = self->writebuf_size;
  char *new_writebuf;

  while (req_size > new_size)
    new_size *= 2;

  if (new_size == self->writebuf_size)
    return self->writebuf;

  if ((new_writebuf = realloc(self->writebuf, new_size)) == NULL)
    return NULL;

  cmdserv_connection_log(self, CMDSERV_INFO,
                         "Increased writebuf_size from %ld to %ld octets",
                         (long)self->writebuf_size, (long)new_size);

  self->writebuf_size = new_size;
  self->writebuf      = new_writebuf;

  return self->writebuf;
}
