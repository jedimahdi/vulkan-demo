#include "base.h"
#include <stdarg.h>
#include <stdio.h>

void _log(LogLevel level, const char* fmt, ...) {
  char* level_string[] = {
      [LOG_LEVEL_INFO] = "INFO",
      [LOG_LEVEL_WARN] = "WARN",
      [LOG_LEVEL_ERROR] = "ERROR",
      [LOG_LEVEL_PANIC] = "PANIC",
  };

  fprintf(stderr, "%s: ", level_string[level]);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}
