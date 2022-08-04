
#include "CMessage.h"
#include "CMessage.h"

int wmain()
{
	CMessage* pMessage = CMessage::Alloc();
	
	*pMessage << (WORD)2 << (BYTE)true << (UINT64)1;


	std::cout << CMessage::GetAllocCount();
	
	pMessage->Free();
}
