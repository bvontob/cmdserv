#include "cmdserv_helpers.h"

#include <stdio.h>

#define CMDSERV_DURATION_DAYS_LEN 21

static char duration[  /* - */    1
                     + /* days */ CMDSERV_DURATION_DAYS_LEN
                     + /* time */ 8
                     + /* nul */  1];

char* cmdserv_duration_str(time_t begin, time_t end) {
  char* strp = duration;
  time_t dur = end - begin;
  time_t days;
  int len;

  if (dur < 0) {
    *(strp++) = '-';
    dur = -dur;
  }

  days = dur / 86400;

  if (days > 0) {
    len = snprintf(strp,
                   CMDSERV_DURATION_DAYS_LEN + 1,
                   "%lldd ",
                   (long long int)days);

    if (len > CMDSERV_DURATION_DAYS_LEN)
      len = sprintf(strp, "?d ");

    strp += len;
    dur  -= days * 86400;
  }

  sprintf(strp, "%02d:%02d:%02d",
          (int)(dur / 3600),
          (int)((dur % 3600) / 60),
          (int)((dur % 3600) % 60));

  return duration;
}
