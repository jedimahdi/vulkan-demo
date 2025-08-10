#pragma once

#define FORMAT_CHECK(fmt_pos, args_pos) __attribute__((format(printf, fmt_pos, args_pos)))

typedef enum {
  LOG_LEVEL_INFO = 0,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_PANIC,
} LogLevel;

void _log(LogLevel level, const char* fmt, ...) FORMAT_CHECK(2, 3);

#define LOG_INFO(...) _log(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...) _log(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) _log(LOG_LEVEL_ERROR, __VA_ARGS__)
