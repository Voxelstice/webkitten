#pragma once

typedef enum {
    LOGGER_INFO = 0,       // Info logging, used for program execution info
    LOGGER_WARNING = 1,    // Warning logging, used on recoverable failures
    LOGGER_ERROR = 2,      // Error logging, used on unrecoverable failures
} LoggerLogLevel;

void Logger_init();
void Logger_writeToLog(const char* text);
void Logger_close();

int Logger_logType(int logType);
void Logger_log(int logType, const char *text, ...);

void Logger_logI(const char *text, ...);
void Logger_logW(const char *text, ...);
void Logger_logE(const char *text, ...);

const char *MsgArgs(const char *text, ...);