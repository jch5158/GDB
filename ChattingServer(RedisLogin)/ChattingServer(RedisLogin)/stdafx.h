#pragma once


#include "RedisConnector/RedisConnector/CTLSRedisConnector.h"
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <iostream>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <conio.h>
#include <locale.h>
#include <string>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "PerformanceProfiler/PerformanceProfiler/CPerformanceProfiler.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "TPSProfiler/TPSProfiler/CTPSProfiler.h"
#include "Parser/Parser/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "HardwareProfiler/HardwareProfiler/CHardwareProfiler.h"

#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "RingBuffer/RingBuffer/CTemplateQueue.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"

#include "GodDamnBug/CommonProtocol.h"
#include "NetworkEngine/NetServerEngine/NetServer/CNetServer.h"
#include "NetworkEngine/LanClientEngine/LanClientEngine/CLanClient.h"

#include "CPlayer.h"
#include "CJob.h"
#include "CLanLoginClient.h"

#include "CChattingServer.h"
#include "CLanMonitoringClient.h"

#pragma comment (lib,"Ws2_32.lib")
#pragma comment (lib,"Winmm.lib")
#pragma comment(lib,"Mswsock.lib")


