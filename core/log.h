extern void logprintf(const char *format, ...);

#define TorqueLogMessage(logType, message)  { logprintf message ; }
#define TorqueLogMessageFormatted(logType, message) { logprintf message; }
#define TorqueLogEnable(logType, enabled) { }
#define TorqueLogBlock(logType, code) { }

int _log_index = 0;

void logprintf(const char *format, ...)
{
	char buffer[4096];
	uint32 bufferStart = 0;
	va_list s;
	va_start( s, format );
	int32 len = vsnprintf(buffer + bufferStart, sizeof(buffer) - bufferStart, format, s);
	printf("LOG %d: %s\n", _log_index, buffer);
	va_end(s);
	fflush(stdout);
}

