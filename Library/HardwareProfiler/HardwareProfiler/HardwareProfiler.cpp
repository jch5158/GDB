#include "stdafx.h"



int main()
{
    CHardwareProfiler::SetHardwareProfiler(TRUE, TRUE, TRUE, TRUE, TRUE, L"Intel[R] Wi-Fi 6 AX201 160MHz");



    for (;;)
    {
        CHardwareProfiler::GetInstance()->UpdateHardwareProfiler();

        wprintf_s(L"Private Bytes : %llf\n", CHardwareProfiler::GetInstance()->GetPrivateBytes());

        wprintf_s(L"Nonpaged Pool : %d\n", (INT)CHardwareProfiler::GetInstance()->GetNonpagedPool()/1000000);

        wprintf_s(L"Available Memory : %d\n", (INT)CHardwareProfiler::GetInstance()->GetAvailableMegaBytes());
        
        wprintf_s(L"Send Bytes : %d\n",(DWORD) CHardwareProfiler::GetInstance()->GetSendBytes());

        wprintf_s(L"Recv Bytes : %d\n\n\n", (DWORD)CHardwareProfiler::GetInstance()->GetRecvBytes());


        Sleep(1000);
    }

    return 0;
}