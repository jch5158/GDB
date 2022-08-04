#pragma once


#include <WS2tcpip.h>

#include "RedisLibrary/RedisLibrary/CTLSRedisConnector.h"
#include <iostream>
#include <Windows.h>
#include <process.h>
#include <time.h>
#include <conio.h>

#include "CommonProtocol.h"

#include "DumpLibrary/DumpLibrary/CCrashDump.h"
#include "SystemLogLibrary/SystemLogLibrary/CSystemLog.h"
#include "ParserLibrary/ParserLibrary/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "HardwareProfilerLibrary/HardwareProfilerLibrary/CHardwareProfiler.h"
#include "CriticalSectionLibrary/CriticalSectionLibrary/CCriticalSection.h"
#include "SRWLockLibrary/SRWLockLibrary/CSRWLockExclusive.h"
#include "SRWLockLibrary/SRWLockLibrary/CSRWLockShared.h"
#include "MessageLibrary/MessageLibrary/CMessage.h"
#include "RingBufferLibrary/RingBufferLib/CRingBuffer.h"
#include "RingBufferLibrary/RingBufferLib/CTemplateRingBuffer.h"
#include "LockFreeStack/LockFreeStackLib/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueueLib/CLockFreeQueue.h"
#include "LockFreeObjectFreeList/ObjectFreeListLib/CLockFreeObjectFreeList.h"
#include "LockFreeObjectFreeList/ObjectFreeListLib/CTLSLockFreeObjectFreeList.h"

#include "DBConnectorLibrary/DBConnectorLibrary/CDBConnector.h"
#include "DBConnectorLibrary/DBConnectorLibrary/CTLSDBConnector.h"
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