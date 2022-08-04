#include "stdafx.h"



class A
{
public:

	A()
		:num1()
		,num2()
		,num3()
	{}


	~A()
	{}

	void* operator new(size_t size)
	{
		return mFreeListManager.Alloc(size);
	}

	void* operator new[](size_t size)
	{
		return mFreeListManager.Alloc(size);
	}


	void operator delete(void* ptr)
	{
		mFreeListManager.Free(ptr);

		return;
	}

	void operator delete[](void* ptr)
	{
		mFreeListManager.Free(ptr);
	}

private:

	long long num1;
	long long num2;
	long long num3;

	static CLockFreeFreeListManager mFreeListManager;
};

CLockFreeFreeListManager A::mFreeListManager;


class B : public A
{
public:

	B()
		:num4()
		,num5()
	{}

	~B() = default;

	long long num4;
	long long num5;
};


int main()
{

	B* p = new B;

	std::cout << p->num4 << "\n";

	delete[] p;

	return 1;
}
