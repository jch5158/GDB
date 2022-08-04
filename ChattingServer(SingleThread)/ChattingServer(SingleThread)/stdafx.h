#pragma once


#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "Parser/Parser/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "HardwareProfiler/HardwareProfiler/CHardwareProfiler.h"

#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "RingBuffer/RingBuffer/CTemplateQueue.h"
#include "LockFreeFreeList/LockFreeFreeList/CLockFreeFreeListManager.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"

#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "NetworkEngine/NetServerEngine/NetServer/CNetServer.h"
#include "NetworkEngine/LanClientEngine/LanClientEngine/CLanClient.h"

//#include "Protocol.h"
#include "../Protocol/GodDamnBug/CommonProtocol.h"
#include "CPlayer.h"
#include "CJob.h"
#include "CChattingServer.h"
#include "CLanMonitoringClient.h"

#include "ServerController.h"

#pragma comment(lib,"Ws2_32.lib")

