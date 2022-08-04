#pragma once

#include <iostream>
#include <Windows.h>


// 8�ھ� CPU ȯ�濡�� �� ���μ����� CPU ������ 100%�� �ɷ����� 1�ʿ� ���ð��� 8�ʰ� ���;� �� ���μ����� 100% CPU ������ �����ش�.

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


	// PC�� CPU ����
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


	// ���μ����� CPU ����
	void UpdateProcessProfile(void)
	{
		ULARGE_INTEGER noneUse    = { 0, };
		ULARGE_INTEGER nowTime    = { 0, };

		ULARGE_INTEGER kernelTime = { 0, };
		ULARGE_INTEGER userTime   = { 0, };


		// ���� �ð��� 100���� ������ �ػ󵵸� ����
		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);

		GetProcessTimes(GetCurrentProcess(), (LPFILETIME)&noneUse, (LPFILETIME)&noneUse, (LPFILETIME)&kernelTime, (LPFILETIME)&userTime);
		
		// �����ð����� �󸶸�ŭ�� �ð��� �������� Ȯ���Ѵ�.
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
		
		// ���� PC�� ���μ����� ������ ���Ѵ�.
		mNumberOfProcessors = systemInfo.dwNumberOfProcessors;


		// �����ڿ��� �� �� ȣ�����ش�.
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

