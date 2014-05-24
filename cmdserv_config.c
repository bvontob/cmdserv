#include "cmdserv_config.h"

#include <stddef.h>

struct cmdserv_config cmdserv_config_get_defaults() {
  return (struct cmdserv_config){
    .connections_max     = 16,
    .connections_backlog = 8,
    .port                = 50000,
    .log_handler         = &cmdserv_logger_stderr,
    .log_object          = NULL,
    .connection_config   = cmdserv_connection_config_get_defaults()
  };
}
