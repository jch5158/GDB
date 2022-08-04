#pragma once

#pragma comment(lib,"Ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <iostream>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include <unordered_map>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "Parser/Parser/CParser.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "PerformanceProfiler/PerformanceProfiler/CPerformanceProfiler.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "RingBuffer/RingBuffer/CTemplateQueue.h"
#include "Message/Message/CMessage.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "CNetServer.h"
#include "CEcho.h"
#include "ServerControl.h"