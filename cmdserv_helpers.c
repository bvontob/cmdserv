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


#define LOGSAFE_BUFLEN 512

static char logsafe_string[LOGSAFE_BUFLEN];

char* cmdserv_logsafe_str(const char *s) {
  size_t i;

  for (i = 0; *s != '\0' && i < (LOGSAFE_BUFLEN - (4 + 3 + 1)); s++) {
    if (*s >= ' ' && *s <= '~') {
      if (*s == '\\')
        logsafe_string[i++] = '\\';
      logsafe_string[i++] = *s;
    } else {
      logsafe_string[i++] = '\\';
      logsafe_string[i++] = ((*s)        >> 6) + '0';
      logsafe_string[i++] = ((*s & 0070) >> 3) + '0';
      logsafe_string[i++] = ((*s & 0007)     ) + '0';
    }
  }

  if (*s != '\0') {
    logsafe_string[i++] = '.';
    logsafe_string[i++] = '.';
    logsafe_string[i++] = '.';
  }
  
  logsafe_string[i] = '\0';
  
  return logsafe_string;
}
