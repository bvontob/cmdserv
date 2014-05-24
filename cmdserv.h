/**
 * @mainpage
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 * @section DESCRIPTION
 *
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
 * @see cmdserv.h cmdserv_connection.h
 */

/**
 * @file cmdserv.h
 *
 * The main cmdserv server object, the only thing you need to include.
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 */

#ifndef CMDSERV_H
#define CMDSERV_H

#include <inttypes.h>
#include <stdarg.h>
#include <sys/select.h>

#include "cmdserv_config.h"
#include "cmdserv_logger.h"
#include "cmdserv_connection.h"


/**
 * The main object representing one server instance.
 *
 * @see cmdserv_start()
 */
typedef struct cmdserv cmdserv;


/**
 * The constructor for a new cmdserv server instance.
 *
 * Returns NULL on failure with errno set by an underlying library.
 *
 * @param config
 *
 *     A cmdserv server configuration structure. You can create one
 *     with reasonable defaults set using
 *     cmdserv_config_get_defaults().
 *
 * @return A cmdserv server instance or NULL on failure.
 *
 * @see cmdserv_config cmdserv_config_get_defaults()
 */
cmdserv* cmdserv_start(struct cmdserv_config config);


/**
 * Handle connections.
 *
 * This is the method you need to call in your main loop to allow
 * cmdserv to handle client connections for you.  It's a replacement
 * for the select() call.
 *
 * @todo Finish documentation and probably change name of method.
 *
 * @param serv
 *
 *     The cmdserv server instance that should handle client
 *     connections.
 *
 * @param timeout
 *
 *     Return after this time has passed even if nothing happened. See
 *     select() for your system.
 */
void cmdserv_sleep(cmdserv* serv, struct timeval *timeout);


/**
 * Shutdown server and free resources.
 *
 * @param serv
 *
 *     The cmdserv server instance to shut down and free.
 */
void cmdserv_shutdown(cmdserv* serv);


/**
 * Log a server-related event.
 * 
 * @see cmdserv_vlog() cmdserv_logseverity cmdserv_connection_log()
 *
 * @param serv
 *
 *     The cmdserv server object to which the logged event applies.
 *
 * @param severity
 *
 *     The severity level for the logged event.
 *
 * @param fmt
 *
 *     The following arguments are the human readable message in
 *     sprintf() format.
 */
void cmdserv_log(cmdserv* serv,
                 enum cmdserv_logseverity severity,
                 const char *fmt, ...);

/**
 * Log a server-related event, va_list version.
 * 
 * @see cmdserv_log() cmdserv_logseverity cmdserv_connection_log()
 *
 * @param serv
 *
 *     The cmdserv server object to which the logged event applies.
 *
 * @param severity
 *
 *     The severity level for the logged event.
 *
 * @param fmt
 *
 *     Message format in printf() format.
 *
 * @param ap
 *
 *     Arguments to format.
 */
void cmdserv_vlog(cmdserv* serv,
                  enum cmdserv_logseverity severity,
                  const char *fmt, va_list ap);


/**
 * Return human-readable status info about this server.
 *
 * The zero-terminated string returned spans multiple lines and is in
 * an unspecified (i.e. not to be parsed) but human-readable format.
 * The line terminator is configurable.  One connection can be marked,
 * if needed.
 *
 * The string buffer is allocated by this method and must be free()'d
 * by the caller after use.
 *
 * Returns NULL on failure and errno should be set by an underlying
 * library in that case.
 *
 * @param serv
 *
 *     The cmdserv server object for which status information should
 *     be reported.
 *
 * @param lt
 *
 *     A line termination string to be used to separate the lines of
 *     the report.  Useful values are probably "\n" or "\r\n",
 *     depending on where you want to use the report.
 *
 * @param mark_conn
 *
 *     In the list of active connections mark the connection with this
 *     ID with an asterisk.  Helpful if you e.g. want to send this
 *     back over one specific connection and the user on it should be
 *     able to quickly identify her own connection.  Use 0 if you
 *     don't want to mark any connections.
 *
 * @return Pointer to zero-terminated string (must be free()'d by the
 *     user) or NULL in case of error.
 */
char *cmdserv_server_status(cmdserv* serv,
                            const char* lt,
                            unsigned long long int mark_conn);


#endif /* CMDSERV_H */
