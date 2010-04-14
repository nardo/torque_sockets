extern void logprintf(const char *format, ...);

#define TorqueLogMessage(logType, message)  { logprintf message ; }
#define TorqueLogMessageFormatted(logType, message) { logprintf message; }
#define TorqueLogEnable(logType, enabled) { }
#define TorqueLogBlock(logType, code) { }

static const char* _log_prefix = "LOG";

void logprintf(const char *format, ...)
{
	char buffer[4096];
	uint32 bufferStart = 0;
	va_list s;
	va_start( s, format );
	int32 len = vsnprintf(buffer + bufferStart, sizeof(buffer) - bufferStart, format, s);
	fprintf(stderr, "%s: %s\n", _log_prefix, buffer);
	va_end(s);
	fflush(stderr);
}

