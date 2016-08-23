#include "cmdserv_connection_config.h"

#include <stddef.h>

struct cmdserv_connection_config cmdserv_connection_config_get_defaults() {
  return (struct cmdserv_connection_config){
    .readbuf_size  = 1024,
    .argc_max      = 8,
    .tokenizer     = CMDSERV_TOKENIZER_DEFAULT,
    .cmd_handler   = NULL,
    .cmd_object    = NULL,
    .open_handler  = NULL,
    .open_object   = NULL,
    .close_handler = NULL,
    .close_object  = NULL,
    .log_handler   = &cmdserv_logger_stderr,
    .log_object    = NULL,
    .client_timeout= 0,
  };
}
