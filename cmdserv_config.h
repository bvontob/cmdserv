/**
 * @file cmdserv_config.h
 *
 * Configuration for the cmdserv server with sane defaults.
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 * A cmdserv server takes quite a few configurable parameters on
 * start-up.  Many users might only want to tweak a few of these
 * (foremost probably the TCP port number the server should listen on
 * and an own handler for commands the server should understand).
 *
 * Instead of having a huge (and possibly ever growing) argument list
 * on the constructor cmdserv_start(), it takes only one single struct
 * cmdserv_config that contains all of the configurable parameters.
 *
 * This file defines the struct and an associated function
 * cmdserv_config_get_defaults() to prefill it.
 *
 * Users then only need to change the few parameters they'd like to
 * tweak and don't need to care about the rest.
 *
 * As an additional bonus, if more configurable parameters get added
 * with future versions, the method cmdserv_config_get_defaults() will
 * always return sane and backwards compatible values for the new
 * parameters.  This means you'll only have to recompile your code
 * against a new version of this library (without thinking about the
 * new features) and everything should still work as before.
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
 * @see cmdserv cmdserv_start()
 */

#ifndef CMDSERV_CONFIG_H
#define CMDSERV_CONFIG_H

#include "cmdserv_connection.h"
#include "cmdserv_connection_config.h"


/**
 * Start-up configuration for a cmdserv server.
 *
 * The structure holding the configurable settings handed over to a
 * new cmdserv server.  You should prefill it using
 * cmdserv_config_get_defaults() and then change the values you'd like
 * to tweak.
 *
 * @see cmdserv_config_get_defaults() cmdserv_start()
 */
struct cmdserv_config {
  /**
   * The maximum number of simultaneous connections the server is
   * willing to accept.
   */
  unsigned int connections_max;

  /**
   * This is the maximum number of pending incoming connections (those
   * that are not yet accepted by the server) that may accumulate. See
   * your systems documentation for listen() for a more detailed
   * explanation.
   */
  unsigned int connections_backlog;

  /**
   * The TCP port number the server should listen on.
   */
  unsigned int port;

  /**
   * The callback the server will send log messages to.
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

  /**
   * The configuration settings for an individual connection.
   *
   * @see cmdserv_connection_config
   */
  struct cmdserv_connection_config connection_config;
};


/**
 * The default configuration for a cmdserv server.
 *
 * The current defaults are to listen on TCP port 50000 and handle a
 * maximum of 16 parallel connections (with a connection backlog of
 * 8). Logging will default to STDERR.
 *
 * All the handlers (except for the logging handler) and handler
 * objects are unset in the defaults.  You need to provide at least a
 * cmd_handler to create a useful server.
 *
 * @return The values to prefill your cmdserv_config struct.
 *
 * @see cmdserv_config cmdserv_connection_config
 */
struct cmdserv_config cmdserv_config_get_defaults();


#endif /* CMDSERV_CONFIG_H */
