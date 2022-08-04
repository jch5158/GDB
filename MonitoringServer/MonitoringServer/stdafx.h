#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <iostream>
#include <Windows.h>
#include <process.h>
#include <conio.h>

#include <locale.h>
#include <unordered_map>
#include <list>

#include "DumpLibrary/DumpLibrary/CCrashDump.h"
#include "SystemLogLibrary/SystemLogLibrary/CSystemLog.h"
#include "ParserLibrary/ParserLibrary/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "HardwareProfilerLibrary/HardwareProfilerLibrary/CHardwareProfiler.h"

#include "DBConnectorLibrary/DBConnectorLibrary/CDBConnector.h"
#include "MessageLibrary/MessageLibrary/CMessage.h"
#include "RingBufferLibrary/RingBufferLib/CRingBuffer.h"
#include "RingBufferLibrary/RingBufferLib/CTemplateRingBuffer.h"
#include "LockFreeObjectFreeList/ObjectFreeListLib/CLockFreeObjectFreeList.h"
#include "LockFreeObjectFreeList/ObjectFreeListLib/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStackLib/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueueLib/CLockFreeQueue.h"
#include "NetworkEngine/NetServerEngine/NetServer/CNetServer.h"
#include "NetworkEngine/LanServerEngine/LanServer/CLanServer.h"

#include "CommonProtocol.h"
#include "ProfileDataType.h"

#include "CNetMonitoringServer.h"
#include "CLanMonitoringServer.h"
#include "ServerControler.h"

#pragma comment(lib,"Ws2_32.lib")

