#pragma once


#include <WS2tcpip.h>
#include <iostream>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <conio.h>

#include "CommonProtocol.h"

#include "CrashDump/CrashDump/CCrashDump.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "Parser/Parser/CParser.h"
#include "RedisConnector/RedisConnector/CTLSRedisConnector.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "HardwareProfiler/HardwareProfiler/CHardwareProfiler.h"
#include "CriticalSection/CriticalSection/CCriticalSection.h"
#include "SRWLock/SRWLock/CSRWLockExclusive.h"
#include "SRWLock/SRWLock/CSRWLockShared.h"
#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "RingBuffer/RingBuffer/CTemplateQueue.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"

#include "DBConnector/DBConnector/CTLSDBConnector.h"
#include "NetworkEngine/NetServerEngine/NetServer/CNetServer.h"
#include "NetworkEngine/LanServerEngine/LanServer/CLanServer.h"
#include "NetworkEngine/LanClientEngine/LanClientEngine/CLanClient.h"

#include "CNetLoginServer.h"
#include "CLanLoginServer.h"
#include "CLanMonitoringClient.h"

#pragma comment (lib,"Ws2_32.lib")
#pragma comment (lib,"cpp_redis.lib")
#pragma comment (lib,"tacopie.lib")
#pragma comment (lib,"Winmm.lib")
