#include "stdafx.h"

int main()
{
	CSerializationBuffer* pBuffer = CSerializationBuffer::Alloc();

	int num = 10;

	//*pBuffer << num;

	int num2;

	try
	{
		*pBuffer >> num2;

		wprintf_s(L"%d\n", num2);
	}
	catch (CSerializationBuffer::CExceptionObject &exception)
	{
		exception.PrintExceptionData();
	}

}

