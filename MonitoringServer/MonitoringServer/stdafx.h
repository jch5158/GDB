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

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"
#include "Parser/Parser/CParser.h"
#include "CPUProfiler/CPUProfiler/CCPUProfiler.h"
#include "HardwareProfiler/HardwareProfiler/CHardwareProfiler.h"

#include "DBConnector/DBConnector/CTLSDBConnector.h"
#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "RingBuffer/RingBuffer/CTemplateQueue.h"
#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"
#include "NetworkEngine/NetServerEngine/NetServer/CNetServer.h"
#include "NetworkEngine/LanServerEngine/LanServer/CLanServer.h"

#include "CommonProtocol.h"
#include "ProfileDataType/GodDamnBug/ProfileDataType.h"

#include "CNetMonitoringServer.h"
#include "CLanMonitoringServer.h"
#include "ServerControler.h"

#pragma comment(lib,"Ws2_32.lib")

