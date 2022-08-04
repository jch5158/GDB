#pragma once


enum eLogList
{
	LOG_LEVEL_DEBUG = 1,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_ERROR
};

#define _LOG(bFilePrintFlag, logLevel,fmt,...)							\
do																		\
{																		\
	if (gLogLevel <= logLevel)											\
	{																	\
		AcquireSRWLockExclusive(&gLogLock);								\
		swprintf_s(gLogBuffer,fmt, ##__VA_ARGS__);						\
		DoPrintLog(bFilePrintFlag,logLevel, gLogBuffer);				\
		ReleaseSRWLockExclusive(&gLogLock);								\
	}																	\
}while(0)


void DoPrintLog(bool bFilePrintFalg, int logLevel, WCHAR* logBuffer);

void SetupLogLevel();

extern int gLogLevel;

extern WCHAR gLogBuffer[1024];

extern SRWLOCK gLogLock;
