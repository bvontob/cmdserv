/**
 * @file cmdserv_connection.h
 *
 * The object representing one client connection.
 *
 * @author    Beat Vontobel <b.vontobel@meteonews.ch>
 * @version   0.9.0
 * @copyright 2014, MeteoNews AG, Beat Vontobel
 *
 * @section DESCRIPTION
 *
 *     XXX
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
 * @see cmdserv.h cmdserv_logger.h
 */

#ifndef CMDSERV_CONNECTION_H
#define CMDSERV_CONNECTION_H

#include <stdarg.h>
#include <sys/types.h>
#include <time.h>

#include "cmdserv_logger.h"
struct cmdserv_connection_config;


/**
 * Reasons on why a connection was closed.
 *
 * These values are communicated to the open_handler and close_handler
 * registered by the application, so it can take special action (if
 * required by the communications protocol implemented) for some
 * "abnormal" reasons for a connection close.
 *
 * The library reserves the values 0, 1, 49x (490 to 499), and 59x
 * (590 to 599) for special use.  Other values, though not part of the
 * enum, but compatible with it (users should stick to positive int
 * values), can theoretically be used by the application in calls to
 * cmdserv_connection_close(), if it wishes to communicate different
 * specific reasons for a close.
 *
 * @see cmdserv_connection_close()
 */
enum cmdserv_close_reason {
  CMDSERV_NO_CLOSE                    = 0,   /**< connection still active   */

  CMDSERV_APPLICATION_CLOSE           = 1,   /**< app initiated close       */

  CMDSERV_CLIENT_DISCONNECT           = 490, /**< client closed connection  */
  CMDSERV_CLIENT_RECEIVE_ERROR        = 491, /**< error from recv()         */
  CMDSERV_CLIENT_TIMEOUT              = 492, /**< client inactivity         */

  CMDSERV_SERVER_SHUTDOWN             = 590, /**< cmdserv_shutdown() called */
  CMDSERV_SERVER_TOO_MANY_CONNECTIONS = 591, /**< connections_max reached   */
};


/**
 * Configure end-of-line character modes.
 *
 * This affects parsing of input, but also the end-of-line characters
 * by methods such as cmdserv_connection_println() or
 * cmdserv_connection_send_status().
 *
 * @todo
 *
 *     Further modes could be added if needed: CMDSERV_LINETERM_CR for
 *     lone CRs (used e.g. on Macs before OS X), and strict modes
 *     CMDSERV_LINETERM_LF_STRICT, CMDSERV_LINETERM_CRLF_STRICT, and
 *     CMDSERV_LINETERM_CR_STRICT. The strict modes would regard any
 *     CR or LF character that's not part of a legal line termination
 *     (and probably also other similar whitespace such as TAB, VTAB,
 *     FF... and maybe all non-printable characters) as a protocol
 *     violation and reject the complete line.
 */
enum cmdserv_lineterm {
  /**
   * Lines are LF terminated.
   *
   * In this mode only LF characters are regarded as special and
   * treated as end-of-line characters.  CRs (be it lone or in a
   * CR-LF-sequence) will be completely ignored by the end-of-line
   * detection and treated as any other non-special character would.
   *
   * Thus a line terminated by a CR-LF-sequence will end in a lone CR
   * after stripping of the LF.  The default command tokenizer will
   * treat the CR as arbitrary white space at the end of the line and
   * silently strip it on tokenization in most cases (thus this mode
   * is quite similar to CMDSERV_LINETERM_LF_OR_CRLF in these cases),
   * in raw line mode though, the handler will get to see the CR (and
   * is thus able to detect if a line was ended by LF or by CRLF).
   */
  CMDSERV_LINETERM_LF,

  /**
   * Lines are terminated by LF or CRLF on input, by CRLF on output.
   *
   * In this mode the line parser will accept a lone LF as line
   * termination as well as a CR-LF-sequence.  Both will be cut off.
   * The information on what terminated the line will be lost even for
   * raw line mode.
   *
   * For sending, all methods that append an end-of-line marker
   * automatically will use CRLF in this mode.
   */
  CMDSERV_LINETERM_CRLF_OR_LF,

  /**
   * Lines are terminated by CRLF.
   *
   * Only a sequence of a CR character directly followed by an LF
   * character will be treated as the end of a line.  All other CR or
   * LF characters appearing in the stream will be completely ignored
   * by the end-of-line detection and handed through to the command
   * tokenizer or directly to the handler (in raw line mode)
   * unchanged.
   */
  CMDSERV_LINETERM_CRLF,
};


/**
 * For future expansion.
 *
 * Currently only one possible treatment is defined.
 *
 * @see cmdserv_connection_command_string()
 */
enum cmdserv_string_treatment {
  CMDSERV_LOG_SAFE = 1,   /**< Escape everything outside printable US-ASCII */
};


/**
 * The object representing one active client connection.
 */
typedef struct cmdserv_connection cmdserv_connection;


/**
 * Accept a new client connection from a listener.
 *
 * This is the constructor for cmdserv connection objects.  Normal
 * users of the library should never have to use it, as the cmdserv
 * main server object takes care of this.  It can be used however to
 * wrap a different main server around just the connection object.
 *
 * Will return NULL on any failure with errno set by an underlying
 * library.
 *
 * @param listener_fd
 *
 *     The file descriptor of the listener socket with the pending
 *     client connection to be accepted.
 *
 * @param conn_id
 *
 *     A unique connection ID to identify this connection.  It will be
 *     used in logging output and in the close_handler callback to
 *     inform the user of this method again when this connection has
 *     been closed.
 *
 * @param config
 *
 *      A cmdserv_connection_config object defining the connection
 *      parameters, including the callbacks.
 *
 * @param close_reason
 *
 *      Set this to a specific reason (the library guarantess to only
 *      ever use CMDSERV_SERVER_TOO_MANY_CONNECTIONS) if it is already
 *      known that this connection will be immediately closed again.
 *      Will usually be 0 (CMDSERV_NO_CLOSE) for ordinary connections.
 *
 * @return The newly created client connection or NULL on failure.
 *
 * @see cmdserv_connection_config
 */
cmdserv_connection
*cmdserv_connection_create(int listener_fd,
                           unsigned long long int conn_id,
                           struct cmdserv_connection_config* config,
                           enum cmdserv_close_reason close_reason);


/**
 * Close a client connection.
 *
 * @param connection
 *
 *     The connection to be closed.
 *
 * @param reason
 *
 *     Why the connection was closed.  This will be communicated to
 *     the close_handler to take appropriate action if needed.  The
 *     library guarantees to only use values defined in the enum
 *     cmdserv_close_reason if it needs to call the close_handler on
 *     its own.
 *
 *     Applications should set this to CMDSERV_APPLICATION_CLOSE (1),
 *     if they do not require to communicate any special reason.  The
 *     value CMDSERV_NO_CLOSE (0) will never be passed on to the
 *     close_handler but translated to CMDSERV_APPLICATION_CLOSE (1)
 *     first by the library.  So if this feature is not used by the
 *     application, any of 0 or 1 can be used.
 *
 * @see enum cmdserv_close_reason
 */
void cmdserv_connection_close(cmdserv_connection* connection,
                              enum cmdserv_close_reason reason);


/**
 * Log a connection-related event.
 *
 * This logging method is analogous to cmdserv_log() but should be
 * used instead of it for events regarding a specific client
 * connection (as opposed to server-wide events).  The logged message
 * will automatically be enriched with information identifying the
 * connection in question.
 * 
 * @see cmdserv_connection_vlog() cmdserv_logseverity cmdserv_log()
 *
 * @param connection
 *
 *     The cmdserv connection object to which the logged event
 *     applies.  Connection information will automatically be added to
 *     the head of the logged message.
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
void __attribute__ ((format (printf, 3, 4)))
cmdserv_connection_log(cmdserv_connection* connection,
                       enum cmdserv_logseverity severity,
                       const char *fmt, ...);

/**
 * Log a connection-related event, va_list version.
 * 
 * @see cmdserv_connection_log() cmdserv_logseverity cmdserv_log()
 *
 * @param connection
 *
 *     The cmdserv connection object to which the logged event
 *     applies.  Connection information will automatically be added to
 *     the head of the logged message.
 *
 * @param severity
 *
 *     The severity level for the logged event.
 *
 * @param fmt
 *
 *     Message format in sprintf() format.
 *
 * @param ap
 *
 *     Arguments to format.
 */
void __attribute__ ((format (printf, 3, 0)))
cmdserv_connection_vlog(cmdserv_connection* connection,
                        enum cmdserv_logseverity severity,
                        const char *fmt, va_list ap);


/**
 * Returns a string representation of the parsed, currently handled
 * command.
 *
 * The zero-terminated string is suitable for logging.  It might be
 * truncated to below 512 characters.  Non-printable characters are
 * replaced by escape sequences.
 *
 * The method is not re-entrant.  It's also only valid to call it from
 * within a command handler.
 *
 * @param trtmt
 *
 *     Processing done on the string.  Currently ignored.
 *
 * @return A zero-terminated string, statically allocated.
 */
char *cmdserv_connection_command_string(cmdserv_connection* connection,
                                        enum cmdserv_string_treatment trtmt);


/**
 * Send a status line on this connection.
 *
 * Status lines are of the format "nnn Message\r\n" where "nnn" is a
 * three-digit numerical code and "Message" is a short human-readable
 * description for the numerical code.
 *
 * This method takes the numerical code separately from the rest of
 * the message (which can be formatted with built-in sprintf()
 * functionality), appends the CR-LF-sequence automatically, and sends
 * the status line down the given connection to the client.
 *
 * @param connection
 *
 *     The cmdserv connection object through which the status line
 *     should be sent.
 *
 * @param status
 *
 *     A three-digit integer status value between 100 and 999. The
 *     definition of status codes is up to the user of this library,
 *     but often status codes as used for many Internet protocols with
 *     2xx being success codes, 4xx being client-error codes, and 5xx
 *     being server-error codes are a good starting point.
 *
 * @param fmt
 *
 *     Further arguments are the human-readable part of the status
 *     line in sprintf() format. The correct line terminator is
 *     automatically appended depending on the current setting from
 *     enum cmdserv_lineterm and should not be added to the string.
 *
 * @return The number of octets actually sent or -1 for errors.
 */
ssize_t __attribute__ ((format (printf, 3, 4)))
cmdserv_connection_send_status(cmdserv_connection* connection,
                               int status,
                               const char *fmt, ...);


/**
 * Send nbyte bytes from the buffer pointed to by buf.
 *
 * This method behaves excactly like your system's send() call, with
 * the only exception being the first argument: A cmdserv connection
 * object instead of a file descriptor.  Refer to your system's
 * send() documentation for further details and semantics.
 *
 * The library itself uses this method as the low-level operation for
 * all output to client connections.
 *
 * @param connection
 *
 *     The cmdserv connection object to send on.
 *
 * @param buf
 *
 *     Pointer to the data buffer from which to write.
 *
 * @param nbyte
 *
 *     Number of bytes to write.
 *
 * @param flags
 *
 *     Flags handed over to send().  Pass in MSG_NOSIGNAL unless you
 *     have installed a signal handler for SIGPIPE.
 *
 * @return The number of octets actually sent or -1 for errors.
 */
ssize_t cmdserv_connection_send(cmdserv_connection* connection,
                                const void *buf,
                                size_t nbyte,
                                int flags);


/**
 * Send a zero-terminated string over the connection.
 *
 * The string will be written without the terminating zero and without
 * a line break.  This is just a tiny convenience wrapper around
 * cmdserv_connection_send(), determining the size of the string using
 * strlen() and calling cmdserv_connection_send() with it.  There's no
 * further overhad associated with this call, as for all the other
 * printf()-like methods.
 *
 * @see cmdserv_connection_println() cmdserv_connection_printf()
 *    cmdserv_connection_send()
 *
 * @param connection
 *
 *     The cmdserv connection object to write on.
 *
 * @param str
 *
 *     A zero-terminated string to write.
 *
 * @return The number of octets actually sent or -1 for errors.
 */
ssize_t cmdserv_connection_print(cmdserv_connection* connection,
                                 const char *str);


/**
 * Write a zero-terminated string followed by a line break.
 *
 * The correct line terminator is automatically appended depending on
 * the current setting from enum cmdserv_lineterm.
 *
 * @see cmdserv_connection_print() cmdserv_connection_printf()
 *     cmdserv_connection_vprintf()
 *
 * @param connection
 *
 *     The cmdserv connection object to write on.
 *
 * @param str
 *
 *     A zero-terminated string to write.
 *
 * @return The number of octets actually sent or -1 for errors.
 */
ssize_t cmdserv_connection_println(cmdserv_connection* connection,
                                   const char *str);


/**
 * Write a formatted string to the connection.
 *
 * @see cmdserv_connection_print() cmdserv_connection_println()
 *     cmdserv_connection_vprintf()
 *
 * @param connection
 *
 *     The cmdserv connection object to write on.
 *
 * @param fmt
 *
 *     The following arguments are exactly as for your systems
 *     printf().
 *
 * @return The number of octets actually sent or -1 for errors.
 */
ssize_t __attribute__ ((format (printf, 2, 3)))
cmdserv_connection_printf(cmdserv_connection* connection,
                          const char *fmt, ...);


/**
 * Write a formatted string to the connection.
 *
 * @see cmdserv_connection_print() cmdserv_connection_println()
 *     cmdserv_connection_printf()
 *
 * @param connection
 *
 *     The cmdserv connection object to write on.
 *
 * @param fmt
 *
 *     The format. As for your system's vprintf().
 *
 * @param ap
 *
 *     Argument list. As for your system's vprintf().
 *
 * @return The number of octets actually sent or -1 for errors.
 */
ssize_t cmdserv_connection_vprintf(cmdserv_connection* connection,
                                   const char *fmt, va_list ap);


/**
 * Retrieve the file descriptor for this connection.
 *
 * External users of the library should probably prefer the intended
 * methods to communicate with the other end of a connection,
 * especially for reading, as you might mess up the internal parser
 * state otherwise.  In some circumstances it might be helpful to have
 * access to the raw file descriptor though for writing (to e.g. hand
 * it over to somebody else), even if a wrapper in between would
 * probably be the cleaner solution.
 *
 * @param connection
 *
 *     The cmdserv connection object for which to retrieve the socket
 *     file descriptor.
 *
 * @return A socket file descriptor.
 */
int cmdserv_connection_fd(cmdserv_connection* connection);


/**
 * Retrieve the connection ID for this connection.
 *
 * @param connection
 *
 *     The cmdserv connection object for which to retrieve the
 *     connection ID.
 *
 * @return The unique (per server) connection ID.
 */
unsigned long long int cmdserv_connection_id(cmdserv_connection* connection);


/**
 * Retrieve idle time for this connection.
 *
 * @param connection
 *
 *     The cmdserv connection object for which to retrieve the
 *     idle time.
 *
 * @return Seconds since last client activity.
 */
time_t cmdserv_connection_time_idle(cmdserv_connection* connection);


/**
 * Retrieve connection time.
 *
 * @param connection
 *
 *     The cmdserv connection object for which to retrieve the
 *     connection time.
 *
 * @return Seconds since this connection was started.
 */
time_t cmdserv_connection_time_connected(cmdserv_connection* connection);


/**
 * Retrieve human-readable client information for this connection.
 *
 * Returns the client address (IP and/or hostname plus port) of the
 * given connection as a human readable string for logging, debugging,
 * error messages, and the like.  No assumption about the format
 * should be made (do not parse this information).
 *
 * The storage for the string pointed to by the return value is
 * allocated dynamically by this method. You need to call free() on
 * the pointer after you're done using it.
 *
 * Returns NULL on failure.
 *
 * @param connection
 *
 *     The cmdserv connection object for which to retrieve the
 *     client information.
 *
 * @return Pointer to client info string (must be free()'d) or NULL.
 */
char *cmdserv_connection_client(cmdserv_connection* connection);


/**
 * Trigger a read on the connection.
 *
 * A call to this method will read available data from the client
 * connection into the internal buffer and parse (and execute through
 * the callback) a command if possible (i.e. if a completed command
 * becomes available throughout this read operation).
 *
 * Normal users of the library should never have to call this method:
 * The main cmdserv server object takes care of this.  The method is
 * exposed to be used by cmdserv, for testing, and for users with
 * special needs who want to use the connection object from a
 * different server.
 *
 * Note that a connection might be closed (and the cmdserv_connection
 * object free()'d!) while in this method.  You should be notified of
 * that through the close_handler() callback.  Make sure this ensures
 * that your code will really get rid of all references to the
 * connection object and will not call anything on it in that case.
 *
 * @see cmdserv
 *
 * @param connection
 *
 *     The cmdserv connection object to read from.
 */
void cmdserv_connection_read(cmdserv_connection* connection);

#endif /* CMDSERV_CONNECTION_H */
