#include "stdafx.h"
#include <iostream>
#include "CTest1.h"

CTest1::CTest1()
	: mData(0x0000000055555555)
	, mCount(0)
{
	static int count;

	//std::cout << "Creator :" << ++count << "\n";
}

CTest1::~CTest1()
{
	static int count;

	//std::cout << "Destructor : " << ++count << "\n";
}

