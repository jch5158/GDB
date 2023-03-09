#pragma once

class CTest
{
public:

	CTest()
	{
		mData = 0x0000000055555555;
		mCount = 0;
	}


	~CTest()
	{
	}

	INT64 mData;

	LONG mCount;

	int arr[500];
};

