/**
 * @file cmdserv_logger.h
 *
 * Common logging functionality for both cmdserv and
 * cmdserv_connection.
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 * The cmdserv and the cmdserv_connection class both implement their
 * own logging method cmdserv_log() and cmdserv_connection_log(),
 * respectively (to be able to automatically add different
 * object-related information). You should be using those from your
 * code.  But their implementation is mostly the same, obviously, so
 * the common "non-object oriented" stuff is defined here.
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
 * @see cmdserv cmdserv_connection
 *     cmdserv_log() cmdserv_connection_log()
 */

#ifndef CMDSERV_LOGGER_H
#define CMDSERV_LOGGER_H


/**
 * The severity levels for the logging methods.
 *
 * @see cmdserv_log() cmdserv_connection_log()
 */
enum cmdserv_logseverity {
  CMDSERV_ERR     = 3, /**< Server errors */
  CMDSERV_WARNING = 4, /**< Client induced errors */
  CMDSERV_INFO    = 6, /**< Connection info */
  CMDSERV_DEBUG   = 7  /**< As it says */
};


/**
 * Translate a logseverity level to a human-readable string.
 *
 * Use this to translate the enum constants from cmdserv_logseverity
 * to an appropriate string constant ("err", "warning"...) for
 * inclusion in logging output.  The returned string must not be
 * free()'d, the pointer is guaranteed to reference a constant object.
 *
 * @param severity
 *
 *     The numeric severity value (enum) to translate.
 *
 * @return A pointer to a string constant for the given log severity.
 *
 * @see cmdserv_logseverity
 */
const char *cmdserv_logseverity_string(enum cmdserv_logseverity severity);


/**
 * The default logger that's used if you don't provide your own.
 *
 * Both cmdserv and cmdserv_connection will use this logger if you
 * don't hand them over your own via their configuration objects.  The
 * default logger will prepend the messages with the string "cmdserv
 * <SEVERITY>: " (where SEVERITY will be replaced with the
 * corresponding severity level as text) and send it to STDERR.
 *
 * @param object
 *
 *     Arbitrary object that can be handed over to your logger
 *     methods. Not used in the default implementation.
 *
 * @param severity
 *
 *     The severity level.
 *
 * @param msg
 *
 *     The message that should be logged.
 *
 * @see cmdserv_config cmdserv_connection_config
 */
void cmdserv_logger_stderr(void* object,
                           enum cmdserv_logseverity severity,
                           const char* msg);

#endif /* CMDSERV_LOGGER_H */
