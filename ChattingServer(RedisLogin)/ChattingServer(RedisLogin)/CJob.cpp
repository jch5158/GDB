#include "stdafx.h"
#include "CJob.h"


CTLSLockFreeObjectFreeList<CJob> CJob::mJobFreeList;

CJob::CJob(void)
	: mJobType(eJobType::CLIENT_JOIN_JOB)
	, mSessionID(-1)
	, mpMessage(nullptr)
	, mLoginResult(FALSE)
	, mAccountNumber(0)
	, mPlayerID{ 0, }
	, mPlayerNickName{ 0, }
	, mSessionKey{ 0, }
{}


CJob* CJob::Alloc(void)
{
	CJob* pJob = mJobFreeList.Alloc();

	return pJob;
}


void CJob::Free(void)
{
	if (mJobFreeList.Free(this) == FALSE)
	{
		CSystemLog::GetInstance()->Log(FALSE, CSystemLog::eLogLevel::LogLevelError, L"[ChattingServer]", L"[Free] Job Free Error");

		CCrashDump::Crash();

		return;
	}

	return;
}
