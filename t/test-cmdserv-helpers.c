/*
 *  test-cmdserv-helpers.c
 *
 *  Copyright (C) 2014  Beat Vontobel <beat.vontobel@futhark.ch>
 *                                                                                 
 *  This program is free software; you can redistribute it and/or                  
 *  modify it under the terms of the GNU General Public License                    
 *  as published by the Free Software Foundation; either version 2                 
 *  of the License, or (at your option) any later version.                         
 *                                                                                 
 *  This program is distributed in the hope that it will be useful,                
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                  
 *  GNU General Public License for more details.                                   
 *                                                                                 
 *  You should have received a copy of the GNU General Public License              
 *  along with this program; if not, write to the Free Software                    
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301, USA.
 *
 */

#include "../cmdserv_helpers.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARGC_MAX 4

int main(void) {
  char *data = NULL;
  size_t len;
  long int begin, end;
  char *token;

  for (int line = 1; getline(&data, &len, stdin) != -1; line++) {
    if ((token = strtok(data, " ")) == NULL)
      errx(EXIT_FAILURE, "Missing begin duration on line %d", line);
    begin = atol(token);

    if ((token = strtok(NULL, " ")) == NULL)
      errx(EXIT_FAILURE, "Missing end duration on line %d", line);
    end = atol(token);

    printf("%ld %ld %s\n",
           begin, end,
           cmdserv_duration_str((time_t)begin, (time_t)end));

    free(data);
    data = NULL;
  }
  
  free(data);
}
