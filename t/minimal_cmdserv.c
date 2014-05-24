/**
 * @file minimal_cmdserv.c
 *
 * The (almost absolute) minimum to get cmdserv running.
 *
 * @author  Beat Vontobel <b.vontobel@meteonews.ch>
 * @see     cmdserv
 * @license This example is put into the public domain
 *
 * This is a minimal example of a cmdserv program to get you started:
 * Once launched it just sits there forever, accepting connections and
 * echoing back all parsed commands to the client.
 */

#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "../cmdserv.h"

void echo_handler(void*, cmdserv_connection*, int argc, char **argv);


/**
 * Launch a minimal server (with the command handler defined below) in
 * an endless loop.
 */
int main(void) {
  cmdserv* server = NULL;
  struct cmdserv_config config = cmdserv_config_get_defaults();
  struct timeval timeout = { .tv_sec  = 1, .tv_usec = 0 };

  config.connection_config.cmd_handler = &echo_handler;

  if ((server = cmdserv_start(config)) == NULL)
    err(EXIT_FAILURE, "cmdserv_start()");

  for (;;)
    cmdserv_sleep(server, &timeout);
}


/**
 * A minimal command handler that just echoes all parsed commands back
 * to the client.
 */
void echo_handler(void *cmd_object,
                  cmdserv_connection* connection,
                  int argc, char **argv) {

  for (; *argv != NULL; (argv)++)
    cmdserv_connection_printf(connection, "'%s' ", *argv);
  cmdserv_connection_printf(connection, "\r\n");

  cmdserv_connection_send_status(connection, 200, "OK");
}
