#include "cmdserv_connection_config.h"

#include <stddef.h>

struct cmdserv_connection_config cmdserv_connection_config_get_defaults() {
  return (struct cmdserv_connection_config){
    .cmd_handler   = NULL,
    .cmd_object    = NULL,
    .open_handler  = NULL,
    .open_object   = NULL,
    .close_handler = NULL,
    .close_object  = NULL,
    .log_handler   = &cmdserv_logger_stderr,
    .log_object    = NULL,
  };
}
