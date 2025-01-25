#include <stdio.h>
#include <time.h>
// TODO: Make this compatible with Linux and MacOS. They dont appear to have such library
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdarg>

// Define this flag to make this not start a file stream
#define LOGGER_NO_FILE

typedef enum {
    LOGGER_INFO = 0,       // Info logging, used for program execution info
    LOGGER_WARNING = 1,    // Warning logging, used on recoverable failures
    LOGGER_ERROR = 2,      // Error logging, used on unrecoverable failures
} LoggerLogLevel;

std::ofstream logStream;

// Custom logging function
std::string CustomLogString(int msgType, const char* text, va_list args) {
    char timeStr[64] = { 0 };
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);

    strftime(timeStr, sizeof(timeStr), "%d-%m-%Y %H:%M:%S", tm_info);
    std::string result = "[" + std::string(timeStr) + "] ";

    switch (msgType) {
        case LOGGER_INFO: result += "[INFO] : "; break;
        case LOGGER_ERROR: result += "[ERROR]: "; break;
        case LOGGER_WARNING: result += "[WARN] : "; break;
        default: break;
    }

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), text, args);
    result += buffer;
    result += "\n";

    return result;
}

void CustomLog(int msgType, const char *text, va_list args) {
    std::string logString = CustomLogString(msgType, text, args);
    printf("%s", logString.c_str());
    #ifndef LOGGER_NO_FILE
        logStream << logString.c_str();
    #endif
}

// probably nonfunctional
const char* MsgArgs(const char* text, ...) {
    va_list args;
    va_start(args, text);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);
    return (const char*) buffer;
}

// This is just a wrapper for the logger in environments where we REALLY don't want raylib to be used
int Logger_logType(int logType) {
    if (logType == 0) {
        return LOGGER_INFO;
    } else if (logType == 1) {
        return LOGGER_WARNING;
    } else if (logType == 2) {
        return LOGGER_ERROR;
    } else {
        return LOGGER_INFO;
    }
}
void Logger_log(int logType, const char *text, ...) {
    va_list args;

    va_start(args, text);
    CustomLog(logType, text, args);
    va_end(args);
}

// A quick and dirty way to have info right away
void Logger_logI(const char *text, ...) {
    va_list args;

    va_start(args, text);
    CustomLog(LOGGER_INFO, text, args);
    va_end(args);
}
void Logger_logW(const char *text, ...) {
    va_list args;

    va_start(args, text);
    CustomLog(LOGGER_WARNING, text, args);
    va_end(args);
}
void Logger_logE(const char *text, ...) {
    va_list args;

    va_start(args, text);
    CustomLog(LOGGER_ERROR, text, args);
    va_end(args);
}

void Logger_init() {
    //SetTraceLogCallback(CustomLog);

    // Create the logs directory
    #ifndef LOGGER_NO_FILE
        std::filesystem::create_directory("./logs");

        // Begin the file stream
        char timeStr[64] = { 0 };
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);

        strftime(timeStr, sizeof(timeStr), "logs/%d-%m-%Y_%H-%M-%S.log", tm_info);
        logStream = std::ofstream(timeStr);
        printf("You might want to look at the actual log file if the game happens to crash\n");
    #else
        printf("!!! WARNING !!! Logs are not gonna be saved to a file.\n");
    #endif
}

void Logger_writeToLog(const char* text) {
    #ifndef LOGGER_NO_FILE
        logStream << text;
    #else
        // it's not gonna write any shit we might need, especially external. So just use raylib for that purpose to be safe
        Logger_log(LOGGER_INFO, text);
        //TraceLog(LOG_INFO, text);
    #endif
}

void Logger_close() {
    #ifndef LOGGER_NO_FILE
        Logger_writeToLog("Logger flushed and closed");
        logStream.flush();
        logStream.close();
        printf("You might want to look at the actual log file if the game happens to crash\n");
    #else
        Logger_writeToLog("Logger closed");
        printf("!!! WARNING !!! Logs are not gonna be saved to a file. Save it now while you have the chance.\n");
    #endif
}