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
				wprintf_s(L"Q �Է� �� ���� ���� \n");
				wprintf_s(L"L �Է� �� �Է¸�� ����\n");

				mbControlModeFlag = true;
			}

			if (controlKey == L'd' || controlKey == L'D')
			{
				CCrashDump::Crash();
			}

			if (controlKey == L'l' || controlKey == L'L')
			{
				wprintf(L"�Է� ��� ����\n");
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

