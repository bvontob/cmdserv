/**
 * @file cmdserv_connection_config.h
 *
 * Configuration for a cmdserv connection with sane defaults.
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 * A cmdserv connection takes quite a few configurable parameters on
 * start-up.  Instead of having a huge (and possibly ever growing)
 * argument list on the constructor cmdserv_connection_create(), it
 * takes only one single struct cmdserv_connection_config that
 * contains all of the configurable parameters.
 *
 * This file defines the struct and an associated function
 * cmdserv_connection_config_get_defaults() to prefill it.
 *
 * Users then only need to change the few parameters they'd like to
 * tweak and don't need to care about the rest.  This is the same idea
 * as for the cmdserv server object.
 *
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
 * @see cmdserv_connection cmdserv_connection_create()
 *      cmdserv_config.h
 */

#ifndef CMDSERV_CONNECTION_CONFIG_H
#define CMDSERV_CONNECTION_CONFIG_H

#include "cmdserv_connection.h"


/**
 * Start-up configuration for a cmdserv_connection.
 *
 * The structure holding the configurable settings handed over to a
 * new cmdserv_connection.  You should prefill it using
 * cmdserv_connection_config_get_defaults() and then change the values
 * you'd like to tweak.
 *
 * @see cmdserv_connection_config_get_defaults()
 *      cmdserv_connection_create()
 */
struct cmdserv_connection_config {
  /**
   * The read-buffer size per client connection.
   *
   * One command must fit into the buffer completely. Thus, this setting
   * limits the maximum length of a command in octets (including the
   * line terminators).
   */
  size_t readbuf_size;

  /**
   * The maximum number of arguments a command may consist of.
   *
   * The command itself counts as an argument (arg0), thus argc_max
   * should be set to 3 to parse a command like "command arg1 arg2".
   */
  unsigned int argc_max;

  /**
   * The connection object will do a callback to this handler
   * whenever a command from the client has been parsed and is ready
   * to execute.
   */
  void (*cmd_handler)(void *cmd_object,
                      cmdserv_connection* connection,
                      int argc,
                      char **argv);
  
  /**
   * A pointer to an arbitrary object that will be handed over to you
   * again in your cmd_handler callback as the first argument. Set it
   * to NULL if you don't need that.
   */
  void *cmd_object;

  /**
   * This callback is called for each new connection accepted.
   *
   * This would be the right place to display e.g. a connection
   * banner.
   *
   * The parameter enum cmdserv_close_reason reason will normally be
   * set to 0 (CMDSERV_NO_CLOSE).  It might be set to
   * CMDSERV_SERVER_TOO_MANY_CONNECTIONS, though, to notify the
   * open_handler that the connection will be closed again right away,
   * so the handler can possibly take different action (like
   * displaying a different banner).
   */
  void (*open_handler)(void *open_object,
                       cmdserv_connection* connection,
                       enum cmdserv_close_reason reason);

  /**
   * A pointer to an arbitrary object that will be handed over to you
   * again in your open_handler callback as the first argument. Set
   * it to NULL if you don't need that.
   */
  void *open_object;

  /**
   * The connection object will inform its creator when the connection
   * is being closed through this callback.  The callback will get
   * triggered right before the close on the socket file descriptor.
   * After the callback has returned the file descriptor is invalid
   * and no methods on this object shall be called, as it destructs
   * itself automatically.
   */
  void (*close_handler)(void *close_object,
                        cmdserv_connection* connection,
                        enum cmdserv_close_reason reason);

  /**
   * A pointer to an arbitrary object that will be handed over to you
   * again in your close_handler callback as the first argument. Set
   * it to NULL if you don't need that.
   */
  void *close_object;

  /**
   * The callback the connection will send log messages to.
   *
   * The default is a built-in method that logs to STDERR.
   */
  void (*log_handler)(void *log_object,
                      enum cmdserv_logseverity severity,
                      const char *msg);

  /**
   * A pointer to an arbitrary object that will be handed over to you
   * again in your log_handler callback as the first argument. Set it
   * to NULL if you don't need that.
   */
  void *log_object;
};


/**
 * The default configuration for a cmdserv_connection.
 *
 * All the handlers (except for the log handler, that we direct to
 * STDERR) and handler objects are unset in the defaults.  You need to
 * provide at least a cmd_handler to create a useful connection.
 *
 * @return The values to prefill your cmdserv_connection_config
 *     struct.
 *
 * @see cmdserv_connection_config
 */
struct cmdserv_connection_config cmdserv_connection_config_get_defaults();


#endif /* CMDSERV_CONNECTION_CONFIG_H */
