#pragma once

#include <iostream>
#include <Windows.h>


// 8코어 CPU 환경에서 내 프로세스의 CPU 사용률이 100%가 될려면은 1초에 사용시간이 8초가 나와야 내 프로세스는 100% CPU 사용률을 보여준다.

class CCPUProfiler
{
public:

	static CCPUProfiler* GetInstance(void)
	{
		static CCPUProfiler cpuProfiler;

		return &cpuProfiler;
	}

	double GetProcessorTotalPercentage(void) const
	{
		return mProcessorTotalPercentage;
	}

	double GetProcessorKernelPercentage(void) const
	{
		return mProcessorKernelPercentage;
	}

	double GetProcessorUserPercentage(void) const
	{
		return mProcessorUserPercentage;
	}

	double GetProcessTotalPercentage(void) const
	{
		return mProcessTotalPercentage;
	}

	double GetProcessKernelPercentage(void) const
	{
		return mProcessKernelPercentage;
	}

	double GetProcessUserPercentage(void) const
	{
		return mProcessUserPercentage;
	}


	// PC의 CPU 사용률
	void UpdateProcessorsProfile(void)
	{
		ULARGE_INTEGER idleTime   = { 0, };
		ULARGE_INTEGER kernelTime = { 0, };
		ULARGE_INTEGER userTime   = { 0, };

		if (GetSystemTimes((PFILETIME)&idleTime, (PFILETIME)&kernelTime, (PFILETIME)&userTime) == false)
			return;

		unsigned long long idleDeltaTime   = idleTime.QuadPart - mProcessorLastIdleTime.QuadPart;
		unsigned long long kernelDeltaTime = kernelTime.QuadPart - mProcessorLastKernelTime.QuadPart;
		unsigned long long userDeltaTime   = userTime.QuadPart - mProcessorLastUserTime.QuadPart;

		unsigned long long totalDeltaTime  = kernelDeltaTime + userDeltaTime;

		if (totalDeltaTime == 0)
		{
			mProcessorTotalPercentage  = 0.0f;
			mProcessorKernelPercentage = 0.0f;
			mProcessorUserPercentage   = 0.0f;
		}
		else
		{
			mProcessorTotalPercentage  = (double)(totalDeltaTime - idleDeltaTime) / totalDeltaTime * 100.0;
			mProcessorKernelPercentage = (double)(kernelDeltaTime - idleDeltaTime) / totalDeltaTime * 100.0;
			mProcessorUserPercentage   = (double)userDeltaTime / totalDeltaTime * 100.0f;
		}

		mProcessorLastIdleTime		   = idleTime;
		mProcessorLastKernelTime	   = kernelTime;
		mProcessorLastUserTime		   = userTime;

		return;
	}


	// 프로세스의 CPU 사용률
	void UpdateProcessProfile(void)
	{
		ULARGE_INTEGER noneUse    = { 0, };
		ULARGE_INTEGER nowTime    = { 0, };

		ULARGE_INTEGER kernelTime = { 0, };
		ULARGE_INTEGER userTime   = { 0, };


		// 현재 시간을 100나노 세컨드 해상도를 가진
		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);

		GetProcessTimes(GetCurrentProcess(), (LPFILETIME)&noneUse, (LPFILETIME)&noneUse, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime);
		
		// 지난시간에서 얼마만큼의 시간이 지났는지 확인한다.
		unsigned long long deltaTime		  = nowTime.QuadPart - mProcessLastTime.QuadPart;

		unsigned long long kernelDeltaTime	  = kernelTime.QuadPart - mProcessLastKernelTime.QuadPart;
		unsigned long long userDeltaTime	  = userTime.QuadPart - mProcessLastUserTime.QuadPart;								  
		unsigned long long processDeltaTime   = kernelDeltaTime + userDeltaTime;
								  
								  
		mProcessTotalPercentage	  = (double)processDeltaTime / mNumberOfProcessors / deltaTime * 100.0;
		mProcessKernelPercentage  = (double)kernelDeltaTime / mNumberOfProcessors / deltaTime * 100.0;	
		mProcessUserPercentage	  = (double)userDeltaTime / mNumberOfProcessors / deltaTime * 100.0;
								  
		mProcessLastTime		  = nowTime;
		mProcessLastKernelTime	  = kernelTime;
		mProcessLastUserTime	  = userTime;

		return;
	}


private:

	CCPUProfiler(void)
		: mNumberOfProcessors(0)

		, mProcessorTotalPercentage(0.0)
		, mProcessorKernelPercentage(0.0)		
		, mProcessorUserPercentage(0.0)
		
		, mProcessTotalPercentage(0.0)
		, mProcessKernelPercentage(0.0)
		, mProcessUserPercentage(0.0)

		, mProcessorLastIdleTime{ 0, }
		, mProcessorLastKernelTime{ 0, }
		, mProcessorLastUserTime{ 0, }

		, mProcessLastTime{ 0, }
		, mProcessLastKernelTime{ 0, }
		, mProcessLastUserTime{ 0, }

	{
		SYSTEM_INFO systemInfo = { 0, };

		GetSystemInfo(&systemInfo);
		
		// 현재 PC의 프로세서의 개수를 구한다.
		mNumberOfProcessors = systemInfo.dwNumberOfProcessors;


		// 생성자에서 한 번 호출해준다.
		UpdateProcessorsProfile();

		UpdateProcessProfile();
	}

	~CCPUProfiler(void) = default;

	CCPUProfiler(const CCPUProfiler&) = delete;
	CCPUProfiler& operator=(const CCPUProfiler&) = delete;


	unsigned int mNumberOfProcessors;

	double mProcessorTotalPercentage;
	double mProcessorKernelPercentage;
	double mProcessorUserPercentage;
	
	double mProcessTotalPercentage;
	double mProcessKernelPercentage;
	double mProcessUserPercentage;
	
	ULARGE_INTEGER mProcessorLastIdleTime;
	ULARGE_INTEGER mProcessorLastKernelTime;
	ULARGE_INTEGER mProcessorLastUserTime;

	ULARGE_INTEGER mProcessLastTime;
	ULARGE_INTEGER mProcessLastKernelTime;
	ULARGE_INTEGER mProcessLastUserTime;
};

