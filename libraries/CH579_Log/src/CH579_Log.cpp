#include "CH579_Log.h"
#include <stdarg.h>
#include <stdio.h>

static LOG_LevelTypeDef s_level  = LOG_INFO;
static Stream          *s_out    = nullptr;
static bool             s_colors = true;

static const char *const s_tag[]   = { "", "E", "W", "I", "D" };
static const char *const s_color[] = { "", "\x1b[31m", "\x1b[33m", "\x1b[32m", "\x1b[34m" };
#define COLOR_RESET "\x1b[0m"

void Log_Init(unsigned long baud, Stream *out)
{
    if (out) {
        s_out = out;
    } else {
        Serial.begin(baud);
        s_out = &Serial;
    }
}

void Log_SetLevel(LOG_LevelTypeDef level) { s_level = level; }
void Log_SetColors(bool enable)           { s_colors = enable; }

void Log_Write(LOG_LevelTypeDef level, const char *tag, const char *fmt, ...)
{
    if (!s_out || level == LOG_NONE || level > s_level) return;

    char buf[LOG_BUFFER_SIZE];
    int len = snprintf(buf, sizeof(buf), "%s[%lu][%s][%s] ",
                       s_colors ? s_color[level] : "",
                       (unsigned long)millis(), s_tag[level], tag);
    if (len < 0) return;
    if (len < (int)sizeof(buf)) {
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(buf + len, sizeof(buf) - (size_t)len, fmt, args);
        va_end(args);
        if (n > 0) len += n;
    }
    if (len > (int)sizeof(buf) - 1) len = (int)sizeof(buf) - 1;

    s_out->write((const uint8_t *)buf, (size_t)len);
    if (s_colors) s_out->print(COLOR_RESET);
    s_out->print("\r\n");
}
