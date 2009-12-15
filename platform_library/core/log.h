extern void logprintf(const char *format, ...);

#define TorqueLogMessage(logType, message)  { printf message ; }
#define TorqueLogMessageFormatted(logType, message) { printf message; printf("\n"); }
#define TorqueLogEnable(logType, enabled) { }
#define TorqueLogBlock(logType, code) { }
