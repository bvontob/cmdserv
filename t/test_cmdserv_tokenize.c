/*
 *  test_cmdserv_tokenize.c
 *
 *    -- test program for the cmdserv_tokenize library. Reads line
 *       after line from stdin and processes each one through
 *       cmdserv_tokenize(). Writes the original line plus the
 *       processed arguments to stdout, in a format that's
 *       human-readable and easily diff'able for test cases.
 * 
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

#include "../cmdserv_tokenize.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#define ARGC_MAX 4

int main(void) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  
  int status = EXIT_SUCCESS;

  int argc = 0;
  char **argv = malloc(ARGC_MAX * sizeof(char*)); /* heap, for valgrind */

  if (argv == NULL)
    err(EXIT_FAILURE, "malloc()");

  while ((read = getline(&line, &len, stdin)) != -1) {
    if (read > 0 && line[read - 1] == '\n')
      line[read - 1] = '\0';

    printf("in:  [%s]\n", line);

    argc = cmdserv_tokenize(line, argv, ARGC_MAX);

    printf("out: ");

    if (argc == -1) {
      printf("Too many arguments\n\n");
    } else {
      int i = 0;
      for (char **arg = argv; *arg != NULL; arg++, i++)
        printf("[%s]", *arg);
      printf("\n");
      if (i != argc) {
        printf("err: i != argc (%d != %d)\n", i, argc);
        status = EXIT_FAILURE;
      }
      printf("\n");
    }
  }
  
  free(argv);
  free(line);
  exit(status);
}
