#pragma once
class CTest
{
public:
	CTest()
	{
		mData = 0x0000000055555555;
		mCount = 0;
	}

	CTest(const CTest& test)
	{
		mData = test.mData;
		mCount = test.mCount;
	}

	CTest& operator=(const CTest& test)
	{
		mData = test.mData;
		mCount = test.mCount;

		return *this;
	}

	~CTest()
	{

	}

	long long mData;

	long mCount;

};

