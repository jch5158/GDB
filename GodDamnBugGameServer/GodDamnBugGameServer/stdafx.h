#pragma once

#define _USE_MATH_DEFINES

#include <WS2tcpip.h>
#include <time.h>
#include <math.h>
#include "RedisConnector/RedisConnector/CTLSRedisConnector.h"
#include "DBConnector/DBConnector/CTLSDBConnector.h"
#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "CriticalSection/CriticalSection/CCriticalSection.h"
#include "PerformanceProfiler/PerformanceProfiler/CTLSPerformanceProfiler.h"
#include "Parser/Parser/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "HardwareProfiler/HardwareProfiler/CHardwareProfiler.h"

#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "RingBuffer/RingBuffer/CTemplateQueue.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"
#include "LockFreeFreeList/LockFreeFreeList/CTLSLockFreeFreeList.h"
#include "LockFreeFreeList/LockFreeFreeList/CLockFreeFreeListManager.h"

#include "CommonProtocol.h"
#include "GodDamnBugDifine.h"
#include "MMOServer/MMOServer/CMMOServer.h"
#include "NetworkEngine/LanClientEngine/LanClientEngine/CLanClient.h"

#include "CDBWriteJob.h"
#include "CLoginDBWriteJob.h"
#include "CLogoutDBWriteJob.h"
#include "CDieDBWriteJob.h"
#include "CCreateCharacterDBWriteJob.h"
#include "CRestartDBWriteJob.h"
#include "CCristalPickDBWriteJob.h"
#include "SRWLock/SRWLock/CSRWLockExclusive.h"
#include "SRWLock/SRWLock/CSRWLockShared.h"
#include "CHPRecoveryDBWriteJob.h"

#include "CLanLoginClient.h"
#include "CGodDamnBug.h"
#include "CLanMonitoringClient.h"
#include "Object/CBaseObject.h"
#include "Object/CPlayer.h"
#include "Object/CCristal.h"
#include "Object/CMonster.h"
#include "CObjectManager.h"

#include <unordered_map>
#include <list>

#pragma comment(lib,"Winmm.lib")