#include "stdafx.h"
#include <process.h>
#include "CTPSProfiler.h"

HANDLE gEvent;



int wmain()
{
	srand((unsigned)time(NULL));

	CreateEvent(NULL, false, false, NULL);

	CTPSProfiler profiler;

	profiler.SetTPSProfiler(L"TEST");

	for (int index = 0; index < 200; ++index)
	{

		profiler.SaveTPSInfo(L"UPDATE TPS", 200);
	}

	if (profiler.PrintTPSProfile() == FALSE)
	{
		std::wcout << L"Failed\n";
	}

	profiler.FreeTPSProfiler();

	return 1;
}
