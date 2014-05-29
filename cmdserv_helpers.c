#include "cmdserv_helpers.h"

#include <string.h> /* strcpy() during development */

static char duration[128];

char* cmdserv_duration_str(time_t begin, time_t end) {
  (void)begin;
  (void)end;
  strcpy(duration, "00:00:00");
  return duration;
}
