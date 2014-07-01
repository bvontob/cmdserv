#include "cmdserv_logger.h"

#define _GNU_SOURCE /* for asprintf() in stdio.h */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *cmdserv_logseverity_string(enum cmdserv_logseverity severity) {
  switch (severity) {
  case CMDSERV_ERR:
    return "err";
  case CMDSERV_WARNING:
    return "warning";
  case CMDSERV_INFO:
    return "info";
  case CMDSERV_DEBUG:
    return "debug";
  default:
    return "?";
  }
}

void cmdserv_logger_stderr(void* object,
                           enum cmdserv_logseverity severity,
                           const char* msg) {
  (void)object; /* UNUSED */

  ssize_t len;
  char *buf;

  /*
   * Instead of using dprintf() or similar means, we build the string
   * in memory and send it to STDERR using write().  This built-in
   * logging to STDERR will mainly be used for debugging, and in our
   * test suite.  And write() should guarantee us an atomic write
   * (dprintf() on the other hand messed up some of the test output).
   */
  if ((len = asprintf(&buf, "cmdserv <%s>: %s\n",
                      cmdserv_logseverity_string(severity),
                      msg))
      > 0) {
    write(STDERR_FILENO, buf, len);
    free(buf);
  }
}
