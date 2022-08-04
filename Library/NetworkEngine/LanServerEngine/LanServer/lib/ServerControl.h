#pragma once



class CServerController
{

public:

	CServerController()
		: mbControlModeFlag(false)
		, mbShutdownFlag(false)
	{

	}

	~CServerController()
	{

	}

	void ServerControling(void)
	{	
		if (_kbhit())
		{
			WCHAR controlKey = _getwch();

			if (controlKey == L'u' || controlKey == L'U')
			{
				wprintf_s(L"Q 입력 시 서버 종료 \n");
				wprintf_s(L"L 입력 시 입력모드 종료\n");

				mbControlModeFlag = true;
			}

			if (controlKey == L'd' || controlKey == L'D')
			{
				CCrashDump::Crash();
			}

			if (controlKey == L'l' || controlKey == L'L')
			{
				wprintf(L"입력 모드 종료\n");
				mbControlModeFlag = false;
			}

			if ((controlKey == L'q' || controlKey == L'Q') && mbControlModeFlag)
			{
				mbShutdownFlag = true;

			}
		}
	}

	bool mbControlModeFlag;

	bool mbShutdownFlag;

};

