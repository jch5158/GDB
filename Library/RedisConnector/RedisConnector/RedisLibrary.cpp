#include "stdafx.h"
#include "CRedisConnector.h"
#include "CTLSRedisConnector.h"


CTLSRedisConnector redis;


unsigned __stdcall WorkerThread(void* pParam)
{
	redis.Connect();

	std::string str;

	for (;;)
	{
		redis.Set("1", "2");

		redis.GetString("1", str);

		std::cout << str << "\n";

		Sleep(100);
	}

	redis.Disconnect();

	return 1;
}


int main()
{
	CRedisConnector::CallWSAStartup();


	// 멀티스레드 테스트
	_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)WorkerThread, nullptr, 0, nullptr);
	_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)WorkerThread, nullptr, 0, nullptr);
	_beginthreadex(nullptr, 0, (_beginthreadex_proc_type)WorkerThread, nullptr, 0, nullptr);

	Sleep(INFINITE);

	CRedisConnector::CallWSACleanup();


	return 1;
}

