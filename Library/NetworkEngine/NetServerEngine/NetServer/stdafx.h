#pragma once

#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib, "DbgHelp.Lib")

#include <WinSock2.h>	
#include <WS2tcpip.h>
#include <strsafe.h>
#include <stdlib.h>
#include <Windows.h>
#include <psapi.h>
#include <tchar.h>
#include <process.h>
#include <time.h>
#include <conio.h>
#include <locale.h>
#include <dbghelp.h>
#include <unordered_map>
#include <list>
#include <stack>

#include "CrashDump/CrashDump/CCrashDump.h"
#include "SystemLog/SystemLog/CSystemLog.h"

#include "LockFreeObjectFreeList/ObjectFreeList/CTLSLockFreeObjectFreeList.h"
#include "Message/Message/CMessage.h"
#include "RingBuffer/RingBuffer/CRingBuffer.h"
#include "LockFreeStack/LockFreeStack/CLockFreeStack.h"
#include "LockFreeQueue/LockFreeQueue/CLockFreeQueue.h"

#include "CNetServer.h"
#include "CContents.h"
#include "lib/ServerControl.h"
