/**
 * @file cmdserv_tokenize.h
 *
 * Parse a string into tokens in ways similar to a shell.
 * 
 * @author    Beat Vontobel <beat.vontobel@futhark.ch>
 * @version   1.0.0
 * @copyright 2014, Beat Vontobel
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
 */

#ifndef CMDSERV_TOKENIZE_H
#define CMDSERV_TOKENIZE_H

/**
 * Parses a string into tokens, splitting at whitespace and handling
 * escapes and quotes, in similar ways as a shell usually does, but
 * without all the magic things like expansion, globbing and more.
 *
 * Special characters are whitespace (see isspace(3)), the
 * double-quote ("), single-quote ('), and the backslash (\):
 * 
 * Single or multiple whitespace characters separate tokens, unless
 * they appear within quotes or are escaped by a backslash.
 * Whitespace at the beginning or the end of the line is completely
 * ignored.
 *
 * Either a double-quote or a single-quote starts a quoted string
 * (both are treated equally) running until an unescaped matching
 * quote character is found.  Whitespace and the respective other
 * quote character loose their special meaning within a quoted string.
 *
 * Every character (special or not, except for the C string terminator
 * ASCII 0) can be escaped by a backslash character.  Special
 * characters, including the backslash itself, loose their special
 * meaning and are used literally, standing for themselves, if
 * escaped.  Characters without a special meaning do not behave
 * differently if escaped and stand always for themselves.
 *
 * Buffers for the function must be provided by the caller.  The
 * zero-terminated string *str is modified in place (call with a copy
 * if you need to preserve the original) and used as the storage space
 * for the individual tokens, but the function guarantees to never use
 * more than the length of the original string.
 *
 * Pointers to the individual tokens are stored in **argv which must
 * be allocated by the caller.  argc_max takes the size of the pointer
 * array.  The function will add NULL as a terminator after the last
 * token in **argv.  Because of that one actual argument less than
 * argc_max can be parsed.
 *
 * The function returns the number of arguments actually parsed (argc)
 * or -1 on error (overflow if more than argc_max minus one arguments
 * were actually available).  The contents of *str and **argv are
 * undefined on error after the function call.
 *
 * @param str
 *
 *     Pointer to the buffer holding a zero-terminated command string
 *     to be tokenized.  Will be overwritten with the parsed
 *     arguments.
 *
 * @param argv
 *
 *     Pointer to the array of zero-terminated argument strings parsed
 *     from the command string.  The storage space for the
 *     pointer-array must be provided by the caller.
 *
 * @param argc_max
 *
 *     The maximum number of arguments (plus one for the terminating
 *     NULL) to parse.
 *
 * @return The number of arguments parsed (argc) or -1 on errors.
 */
int cmdserv_tokenize(char *str, char **argv, int argc_max);

#endif /* CMDSERV_TOKENIZE_H */
