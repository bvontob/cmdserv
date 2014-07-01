#include "clientlib.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAXCONNS  256
#define MAXBUFLEN 256

int main(int argc, char** argv) {
  int fd[MAXCONNS];
  ssize_t conns;
  size_t linelen;
  FILE* stream[MAXCONNS];
  char* line = NULL;
  int grace = 0;

  for (conns = 0; conns < MAXCONNS; conns++) {
    fd[conns] = cmdserv_connect(argc, argv);

    if((stream[conns] = fdopen(fd[conns], "rb")) == NULL)
      err(EXIT_FAILURE, "fdopen(#%d)", fd[conns]);

    free(line);
    line = NULL;

    if (getline(&line, &linelen, stream[conns]) == -1)
      err(EXIT_FAILURE, "getline()");

    printf("%s", line);

    if (strcmp(line, "101 Ready\r\n") != 0) {
      if (grace)
        break;
      grace++;
    }
  }

  free(line);
  line = NULL;

  for (--conns; conns >= 0; conns--) {
    fclose(stream[conns]);

    /*
     * For the test cases: Try to get a stable order on close
     * operations, by checking that the file handle is realy closed,
     * and trying to yield in poor man's way in between. Otherwise the
     * chances are too high, that the cmdserv gets multiple close
     * operations at once and works through them in a different order.
     */
    while (fcntl(fd[conns], F_GETFD) != -1) {
      sleep(0);
    }

    info("#%d closed.", fd[conns]);
  }
}
