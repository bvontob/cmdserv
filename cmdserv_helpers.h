/**
 * @file cmdserv_helpers.h
 *
 * Additional functions used internally by the library.
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

#ifndef CMDSERV_HELPERS_H
#define CMDSERV_HELPERS_H

#include <time.h>

/**
 * Formats the difference between two points in time as a
 * human-readable duration string.
 *
 * Returns strings of the form "00:05:13" (5 minutes and 13 seconds),
 * "5d 10:00:00" (5 days and 10 minutes), "00:00:00" (a zero
 * duration), or "-2d 14:23:15" (negative duration, end before begin).
 * No units larger than days will be used.
 *
 * This function is not re-entrant (it does not interfere with any
 * other use of functions from time.h, though).  The buffer for the
 * returned string is statically allocated.  Do not call free() on it.
 * And note that it will be overwritten by the next call to this
 * function.
 *
 * @param begin
 *
 *     The point in time of the start of the duration.
 *
 * @param end
 *
 *     The point in time of the end of the duration.
 *
 * @return A zero-terminated string representing the duration.
 */
char* cmdserv_duration_str(time_t begin, time_t end);

#endif /* CMDSERV_HELPERS_H */
