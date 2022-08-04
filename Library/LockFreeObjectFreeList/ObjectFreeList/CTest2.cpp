#include "stdafx.h"
#include "Ctest1.h"
#include "CTest2.h"



CTest2::CTest2()
{
	this->check = 10;

	printf_s("CTest2 »ý¼ºÀÚ\n");
}

CTest2::~CTest2()
{
	printf_s("CTest2\n");
}

void CTest2::ShowMe()
{
	printf_s("Ctest2 ShowMe\n");
}
