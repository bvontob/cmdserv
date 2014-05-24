#include "cmdserv_logger.h"

#include <stdio.h>

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

  dprintf(2, "cmdserv <%s>: %s\n",
          cmdserv_logseverity_string(severity),
          msg);
}
