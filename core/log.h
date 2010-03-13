extern void logprintf(const char *format, ...);

#define TorqueLogMessage(logType, message)  { logprintf message ; }
#define TorqueLogMessageFormatted(logType, message) { logprintf message; }
#define TorqueLogEnable(logType, enabled) { }
#define TorqueLogBlock(logType, code) { }
