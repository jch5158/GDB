
#include "CParser.h"

struct stDissession
{
	DWORD messageType;
	UINT64 sessionID;
};

stDissession disArray[53] = { 0, };

UINT64 releaseArray[51] = { 0, };

UINT64 inDisArray[50] = { 0, };

int wmain()
{
	WCHAR buffer[MAX_PATH] = { 0, };
	
	CParser parser;

	if (parser.LoadFile(L"Config//hello.ini") == false)
	{
		wprintf_s(L"Failed\n");
		return 1;
	}

	INT num = 0;

	parser.GetNamespaceString(L"HI",L"hello", buffer, MAX_PATH);

	wprintf_s(L"%s\n", buffer);
}
