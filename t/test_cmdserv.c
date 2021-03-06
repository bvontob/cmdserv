/**
 * @file test_cmdserv.c
 *
 * Test program for the cmdserv library.
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 * This test program for the cmdserv library implements a trivial
 * shell that allows you to set one global value on the server and
 * query it, as well as to retrieve some server status and shutdown
 * the server.  It's used as a simple example on how to use the
 * library, and for test cases.  See the 'help' command from within
 * the shell on how to the "features" of the shell.  You can connect
 * to it with netcat/nc, telnet, or similar programs to play around
 * with it.
 *
 * @section LICENSE
 *
 *     This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of
 *     the License, or (at your option) any later version.
 *                                                                                 
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *                                                                                 
 *     You should have received a copy of the GNU General Public
 *     License along with this program; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *     Boston, MA 02110-1301, USA.
 *
 * @see cmdserv
 */

#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define CMDSERV_TEST_TCP_PORT 12346

#include "../cmdserv.h"

static bool shutdownreq = false;
static cmdserv* server = NULL;

void banner(void *object,
            cmdserv_connection* connection,
            enum cmdserv_close_reason close_reason) {
  (void)object; /* UNUSED */

  switch (close_reason) {
  case CMDSERV_NO_CLOSE:
    cmdserv_connection_send_status(connection, 101, "Ready");
    break;
  case CMDSERV_APPLICATION_CLOSE:
    cmdserv_connection_send_status(connection, 200, "Bye");
    break;
  case CMDSERV_CLIENT_TIMEOUT:
    cmdserv_connection_send_status(connection, close_reason, "Client timeout");
    break;
  case CMDSERV_SERVER_SHUTDOWN:
    cmdserv_connection_send_status(connection, close_reason, "Server shutdown");
    break;
  case CMDSERV_SERVER_TOO_MANY_CONNECTIONS:
    cmdserv_connection_send_status(connection, 500, "Busy");
    break;
  default:
    cmdserv_connection_send_status(connection, close_reason, "Disconnect");
  }
}

void handler(void *cmd_object, cmdserv_connection* connection, int argc, char **argv) {
  (void)cmd_object; /* UNUSED */

  static char value[256] = "";

  if (argc == 0) {
    cmdserv_connection_send_status(connection,
                                   100, "No command");

  } else if (strcmp("help", argv[0]) == 0) { /* help */
    if (argc != 1)
      goto WRONG_ARGUMENTS;

    cmdserv_connection_println(connection, "AVAILABLE COMMANDS:");
    cmdserv_connection_println(connection, "  value set \"string\"");
    cmdserv_connection_println(connection, "      Store string on server (globally).");
    cmdserv_connection_println(connection, "  value get");
    cmdserv_connection_println(connection, "      Retrieve global string from server.");
    cmdserv_connection_println(connection, "  timeout [client timeout in seconds]");
    cmdserv_connection_println(connection, "      Check or change timeout setting.");
    cmdserv_connection_println(connection, "  server status");
    cmdserv_connection_println(connection, "      Display server status.");
    cmdserv_connection_println(connection, "  parse [ARGS...]");
    cmdserv_connection_println(connection, "      Echo back the parsed command string.");
    cmdserv_connection_println(connection, "  server shutdown");
    cmdserv_connection_println(connection, "      Disconnect all clients and shutdown.");
    cmdserv_connection_println(connection, "  exit/quit/disconnect");
    cmdserv_connection_println(connection, "      Terminate your connection.");
    cmdserv_connection_println(connection, "  help");
    cmdserv_connection_println(connection, "      This help text.");
    cmdserv_connection_send_status(connection, 200, "OK");

  } else if (strcmp("exit", argv[0]) == 0 /* exit */
             || strcmp("quit", argv[0]) == 0
             || strcmp("disconnect", argv[0]) == 0) {
    if (argc != 1)
      goto WRONG_ARGUMENTS;
    cmdserv_connection_close(connection, CMDSERV_APPLICATION_CLOSE);

  } else if (strcmp("value", argv[0]) == 0) { /* value get/set */
    if (strcmp("get", argv[1]) == 0) {
      if (argc != 2)
        goto WRONG_ARGUMENTS;

    } else if (strcmp("set", argv[1]) == 0) {
      if (argc != 3)
        goto WRONG_ARGUMENTS;
      strncpy(value, argv[2], sizeof(value));
      value[sizeof(value) - 1] = '\0';

    } else {
      goto WRONG_ARGUMENTS;
    }

    cmdserv_connection_println(connection, value);
    cmdserv_connection_send_status(connection, 200, "OK");
    
  } else if (strcmp("timeout", argv[0]) == 0) { /* timeout */
    if (argc > 2)
      goto WRONG_ARGUMENTS;
    else if (argc == 2)
      cmdserv_connection_set_client_timeout(connection, atol(argv[1]));

    if (cmdserv_connection_client_timeout(connection))
      cmdserv_connection_printf(connection, "Client timeout is %jds",
                                (intmax_t)cmdserv_connection_client_timeout(connection));
    else
      cmdserv_connection_printf(connection, "Client timeout is disabled");
    cmdserv_connection_println(connection, "");
    cmdserv_connection_send_status(connection, 200, "OK");

  } else if (strcmp("parse", argv[0]) == 0) { /* parse */
    cmdserv_connection_send_status(connection, 200,
                                   "%s",
                                   cmdserv_connection_command_string(connection,
                                                                     CMDSERV_LOG_SAFE));

  } else if (strcmp("server", argv[0]) == 0) { /* server control commands */
    if (argc != 2)
      goto WRONG_ARGUMENTS;

    if (strcmp("status", argv[1]) == 0) {
      char *msg = cmdserv_server_status(server,
                                        "\r\n",
                                        cmdserv_connection_id(connection));
      if (msg == NULL) {
        cmdserv_connection_send_status(connection, 500,
                                       "Internal server error");
      } else {
        cmdserv_connection_print(connection, msg);
        cmdserv_connection_send_status(connection, 200, "OK");
      }

    } else if (strcmp("shutdown", argv[1]) == 0) {
      shutdownreq = true;
      cmdserv_connection_send_status(connection, 200, "OK");
    }

  } else { /* Unknown command */
    cmdserv_connection_send_status(connection,
                                   404, "Command '%s' not found",
                                   argv[0]);
  }

  return;

 WRONG_ARGUMENTS:
  cmdserv_connection_send_status(connection,
                                 400, "Wrong arguments for '%s'",
                                 argv[0]);
}

int main(void) {
  struct timeval timeout = { .tv_sec  = 1,
                             .tv_usec = 0 };

  struct cmdserv_config config = cmdserv_config_get_defaults();

  config.port                            = 12346;
  config.connections_max                 = 4;
  config.connection_config.cmd_handler   = &handler;
  config.connection_config.open_handler  = &banner;
  config.connection_config.close_handler = &banner;

  server = cmdserv_start(config);

  if (server == NULL)
    err(EXIT_FAILURE, "failed cmdserv_start()");

  while (!shutdownreq)
    cmdserv_sleep(server, &timeout);

  cmdserv_shutdown(server);
  exit(EXIT_SUCCESS);
}
