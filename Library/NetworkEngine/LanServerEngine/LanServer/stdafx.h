#pragma once


#include <WinSock2.h>	
#include <WS2tcpip.h>
#include <strsafe.h>
#include <iostream>
#include <Windows.h>
#include <psapi.h>
#include <tchar.h>
#include <process.h>
#include <time.h>
#include <conio.h>
#include <locale.h>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"

#include "lib/ServerControl.h"

#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"

#include "CLanServer.h"
#include "CContents.h"

#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"Winmm.lib")