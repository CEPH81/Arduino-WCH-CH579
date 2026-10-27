/*
 * CH579_Log.h - Log por niveles con colores ANSI, estilo esp_log.
 * Portado de Hal_CH57xx_LOG (proyectos Keil de la CH579M).
 *
 *   Log_Init(115200);                 // Serial.begin(115200)
 *   Log_SetLevel(LOG_DEBUG);
 *   LOGI("MAIN", "valor=%d", x);      // [1234][I][MAIN] valor=5   (en verde)
 *
 * Formato: <color>[millis][nivel][tag] mensaje<reset>\r\n
 * Los colores se ven en terminales con ANSI (PuTTY, Tera Term, VS Code);
 * el Monitor Serie del Arduino IDE muestra los codigos de escape tal cual.
 */
#ifndef CH579_LOG_H
#define CH579_LOG_H

#include <Arduino.h>

typedef enum {
    LOG_NONE = 0,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
} LOG_LevelTypeDef;

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 128
#endif

void Log_Init(unsigned long baud = 115200, Stream *out = nullptr);   /* por defecto, Serial */
void Log_SetLevel(LOG_LevelTypeDef level);
void Log_SetColors(bool enable);
void Log_Write(LOG_LevelTypeDef level, const char *tag, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define LOGE(tag, fmt, ...)  Log_Write(LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...)  Log_Write(LOG_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...)  Log_Write(LOG_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...)  Log_Write(LOG_DEBUG, tag, fmt, ##__VA_ARGS__)

#endif
