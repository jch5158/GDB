#pragma once

#include <WinSock2.h>	
#include <WS2tcpip.h>
#include <iostream>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <conio.h>
#include <locale.h>


#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "Parser/Parser/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"

#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"

#include "CNetClient.h"

#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
