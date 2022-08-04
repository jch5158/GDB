#include "stdafx.h"

SRWLOCK gLogLock;

int gLogLevel;

WCHAR gLogBuffer[1024];

void DoPrintLog(bool bFilePrintFlag, int logLevel, WCHAR* logBuffer)
{
	FILE* fp = nullptr;

	static WCHAR logInfo[2000];

	tm stTm;

	__time64_t nowTime;

	time(&nowTime);

	_localtime64_s(&stTm, &nowTime);
	
	switch (logLevel)
	{
	case eLogList::LOG_LEVEL_DEBUG:

		wsprintf(logInfo, L"[%02d/%02d/%02d %02d:%02d:%02d][dfLOG_LEVEL_DEBUG] ", stTm.tm_year + 1900, stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

		wcscat_s(logInfo, logBuffer);

		if (bFilePrintFlag)
		{		
			_wfopen_s(&fp, L"LogData.txt", L"a+t");
			if (fp != nullptr)
			{
				fwprintf_s(fp, logInfo);

				fclose(fp);
			}
		}

		wprintf_s(L"%s", logInfo);

		break;

	case eLogList::LOG_LEVEL_WARNING:

		wsprintf(logInfo, L"[%02d/%02d/%02d %02d:%02d:%02d][LOG_LEVEL_WARNING] ", stTm.tm_year + 1900, stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

		wcscat_s(logInfo, logBuffer);

		if (bFilePrintFlag)
		{		
			_wfopen_s(&fp, L"LogData.txt", L"a+t");
			if (fp != nullptr)
			{
				fwprintf_s(fp, logInfo);

				fclose(fp);
			}
		}


		wprintf_s(L"%s", logInfo);

		break;

	case eLogList::LOG_LEVEL_NOTICE:

		wsprintf(logInfo, L"[%02d/%02d/%02d %02d:%02d:%02d][LOG_LEVEL_NOTICE] ", stTm.tm_year + 1900, stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

		wcscat_s(logInfo, logBuffer);

		if (bFilePrintFlag)
		{
			_wfopen_s(&fp, L"LogData.txt", L"a+t");
			if (fp != nullptr)
			{
				fwprintf_s(fp, logInfo);

				fclose(fp);
			}
		}


		wprintf_s(L"%s", logInfo);

		break;

	case eLogList::LOG_LEVEL_ERROR:

		wsprintf(logInfo, L"[%02d/%02d/%02d %02d:%02d:%02d][LOG_LEVEL_ERROR] ", stTm.tm_year + 1900, stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

		wcscat_s(logInfo, logBuffer);

		if (bFilePrintFlag)
		{
			_wfopen_s(&fp, L"LogData.txt", L"a+t");
			if (fp != nullptr)
			{
				fwprintf_s(fp, logInfo);

				fclose(fp);
			}
		}

		wprintf_s(L"%s", logInfo);

		break;
	default:

		wsprintf(logInfo, L"[%02d/%02d/%02d %02d:%02d:%02d] ", stTm.tm_year + 1900, stTm.tm_mon + 1, stTm.tm_mday, stTm.tm_hour, stTm.tm_min, stTm.tm_sec);

		wcscat_s(logInfo, logBuffer);


		_wfopen_s(&fp, L"LogData.txt", L"a+t");
		if (fp != nullptr)
		{
			fwprintf_s(fp, logInfo);

			fclose(fp);
		}


		wprintf_s(L"%s", logInfo);

		int* ptr = nullptr;

		*ptr = -1;

		break;
	}

	
	return;
}



void SetupLogLevel()
{
	wprintf_s(L"1. LOG_LEVEL_DEBUG\n2. LOG_LEVEL_WARNING\n3. LOG_LEVEL_NOTICE\n4. LOG_LEVEL_ERROR\n");
	wprintf_s(L"로그 레벨을 입력하세요 : ");

	WCHAR buffer[10] = { 0, };
	wscanf_s(L"%s", buffer, (unsigned int)_countof(buffer));

	gLogLevel = _wtoi(buffer);

}