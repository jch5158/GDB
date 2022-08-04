

#include <locale>
#include "CCPUProfiler.h"

#pragma comment(lib,"Winmm.lib")

int wmain()
{
	_wsetlocale(LC_ALL, L"");

	for(;;)
	{
		static DWORD time = timeGetTime();

		if (timeGetTime() - time >= 1000)
		{
			CCPUProfiler::GetInstance()->UpdateProcessProfile();

			wprintf_s(L"Total  : %3.1lf\n\n", CCPUProfiler::GetInstance()->GetProcessTotalPercentage());

			wprintf_s(L"Kernel  : %3.1lf\n\n", CCPUProfiler::GetInstance()->GetProcessKernelPercentage());

			wprintf_s(L"User  : %d\n\n", (DWORD)CCPUProfiler::GetInstance()->GetProcessUserPercentage());

			time = timeGetTime();
		}		
	}

	//wprintf_s(L"%lf", ((float)num2 / (float)num));
}
