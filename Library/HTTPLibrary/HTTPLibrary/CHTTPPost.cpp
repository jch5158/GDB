#include "CHTTPPost.h"

shin::CHTTPPost::CHTTPPost(INT connectTimeout, INT recvTimeout, INT sendTimeout)
	: mRecvTimeout(recvTimeout)
	, mSendTimeout(sendTimeout)
	, mConnectTimeout(connectTimeout)
	, mConnectSocket(INVALID_SOCKET)
{
	if (InitNetwork() == FALSE)
	{
		return;
	}
}


shin::CHTTPPost::~CHTTPPost()
{
	WSACleanup();
}


INT shin::CHTTPPost::POST(const CHAR* pURI, const CHAR* pPostData, CHAR* pRecvData, INT recvBufSize, INT* pOutRecvedDataSize)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Initialization
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	CHAR requestMsg[MAX_REQUEST_MSG_SIZE];
	CHAR URI[MAX_URI_SIZE];
	URI[0] = '\0';
	requestMsg[0] = '\0';

	if (pURI != nullptr)
	{
		SIZE_T uriSize = strlen(pURI);
		memcpy(URI, pURI, uriSize);
		URI[uriSize] = '\0';
	}

	INT postDataSize = 0;
	if (pPostData != nullptr)
	{
		postDataSize = (INT)strlen(pPostData);
	}

	// ————————————————————————————————————————————————————————
	// HTTP POST Header 생성
	// ————————————————————————————————————————————————————————
	CHAR postHeaderTemplate[] = "POST http://%s%s%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: SHIN\r\nConnection: Close\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n";
	sprintf_s(requestMsg, postHeaderTemplate, mHost, mURI, URI, mHost, postDataSize);

	INT sendSize = (INT)strlen(requestMsg) + postDataSize;

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// WebServer Connect후 Request MSG 전송
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	SOCKET connectSocket = Connect();
	if (connectSocket == INVALID_SOCKET)
	{
		return FUNCTION_ERROR;
	}

	INT retval = Send(connectSocket, requestMsg, pPostData);
	if (retval != sendSize)
	{
		return FUNCTION_ERROR;
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Response MSG에서 BODY 획득
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	retval = Recv(connectSocket, pRecvData, recvBufSize, pOutRecvedDataSize);

	closesocket(connectSocket);

	return retval;
}

void shin::CHTTPPost::SetServerURL(std::wstring URL)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Initialization
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	WCHAR hostPart[64];
	WCHAR uriPart[64];
	WCHAR portPart[6];

	hostPart[0] = L'\0';
	uriPart[0] = L'\0';
	portPart[0] = L'\0';

	mURI[0] = L'\0';
	mHost[0] = L'\0';
	mPort = 0;

	WCHAR* pTemp = nullptr;

	WCHAR copyURL[MAX_URL_SIZE];
	SIZE_T urlSize = URL.size() * 2;
	memcpy(copyURL, URL.c_str(), urlSize);
	copyURL[urlSize] = '\0';

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// http:// 이후를 Host 포인터로 잡는다.
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	WCHAR* pHost = wcsstr(copyURL, L"http://");
	if (pHost == nullptr)
	{
		pHost = copyURL;
	}
	else
	{
		// NULL 문자 크기는 뻄
		pHost += _countof(L"http://") - 1;
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// : 이후를 Port 포인터로 잡는다.
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	WCHAR* pPort = wcschr(pHost, L':');
	if (pPort != nullptr)
	{
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		// Host 획득
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		wcsncpy_s(hostPart, pHost, (SIZE_T)(pPort - pHost));

		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		// Port 획득
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		pTemp = wcschr(pPort, L'/');
		if (pTemp != nullptr)
		{
			pPort++;
			wcsncpy_s(portPart, pPort, (SIZE_T)(pTemp - pPort));
			mPort = _wtoi(portPart);
		}
		else
		{
			wcscpy_s(portPart, pPort + 1);
			mPort = _wtoi(portPart);
		}
	}
	else
	{
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		// : 를 찾지 못했기 때문에 Default port(80) 세팅
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		mPort = 80;

		pTemp = nullptr;
		pTemp = wcschr(pHost, L'/');
		if (pTemp != nullptr)
		{
			wcsncpy_s(hostPart, pHost, (SIZE_T)(pTemp - pHost));
		}
		else
		{
			// / 를 찾지 못했다면 그대로 저장
			wcscpy_s(hostPart, pHost);
		}
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// URI 획득
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	WCHAR* pURI = wcschr(pHost, L'/');
	if (pURI != nullptr)
	{
		// wcslen은 NULL 문자를 제외하고 길이를 구해주기 때문에 +1 함
		wcsncpy_s(uriPart, pURI, (SIZE_T)wcslen(pURI) + 1);
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// ServerSockAddr 세팅
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	ZeroMemory(&mServerSockAddr, sizeof(mServerSockAddr));

	mServerSockAddr.sin_family = AF_INET;
	mServerSockAddr.sin_port = htons(mPort);
	DomainToIP(hostPart, &mServerSockAddr.sin_addr);

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// UTF-8로 변환하여 Host, URI 보관
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	UTF16toUTF8(mHost, hostPart, _countof(mHost));
	UTF16toUTF8(mURI, uriPart, _countof(mURI));
}

bool shin::CHTTPPost::InitNetwork(void)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Initialization
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	WSADATA wsa;
	INT retval = NULL;

	retval = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (retval != NO_ERROR)
	{
		LOG(L"System", CSystemLog::eLog::LEVEL_ERROR, L"WSAStartup function failed with error : %d", WSAGetLastError());
		return FALSE;
	}

	return TRUE;
}

SOCKET shin::CHTTPPost::Connect(void)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Socket 생성
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connectSocket == INVALID_SOCKET)
	{
		LOG(L"System", CSystemLog::eLog::LEVEL_ERROR, L"socket function failed with error : %d", WSAGetLastError());
		return FALSE;
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Socket 옵션 적용     : TIMEOUT 옵션
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	INT retval = 0;
	retval = setsockopt(connectSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&mRecvTimeout, sizeof(mRecvTimeout));
	if (retval == SOCKET_ERROR)
	{
		LOG(L"System", CSystemLog::eLog::LEVEL_ERROR, L"setsockopt(SO_RCVTIMEO) function failed with error : %d", WSAGetLastError());
		return INVALID_SOCKET;
	}

	retval = setsockopt(connectSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&mSendTimeout, sizeof(mSendTimeout));
	if (retval == SOCKET_ERROR)
	{
		LOG(L"System", CSystemLog::eLog::LEVEL_ERROR, L"setsockopt(SO_SNDTIMEO) function failed with error : %d", WSAGetLastError());
		return INVALID_SOCKET;
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Make non-blockSocket
	//
	// Connection timeout 기능을 넣기 위함
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	ULONG mode = 1;
	ioctlsocket(connectSocket, FIONBIO, &mode);

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Web server 연결시도
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	retval = connect(connectSocket, (SOCKADDR*)&mServerSockAddr, sizeof(mServerSockAddr));
	if (retval == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			return INVALID_SOCKET;
		}
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Make blockSocket
	//
	// Recv, Send시 blockSocket으로 작동해야하기 때문에 초기화
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	mode = 0;
	ioctlsocket(connectSocket, FIONBIO, &mode);

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Connect timeout만큼 대기
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	TIMEVAL timeout = { mConnectTimeout, 0 };

	fd_set write;
	FD_ZERO(&write);
	FD_SET(connectSocket, &write);

	select(0, nullptr, &write, nullptr, &timeout);
	if (FD_ISSET(connectSocket, &write) == FALSE)
	{
		return INVALID_SOCKET;
	}

	return connectSocket;
}

INT shin::CHTTPPost::Send(SOCKET connectSocket, CHAR* requestMsg, const CHAR* pPostData)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// PostData를 기존 requestMsg 부착후 전송
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	if (pPostData != nullptr)
	{
		strcat_s(requestMsg, MAX_REQUEST_MSG_SIZE, pPostData);
	}

	INT requestMsgSize = (INT)strlen(requestMsg);

	INT retval = send(connectSocket, requestMsg, requestMsgSize, 0);
	if (retval == SOCKET_ERROR)
	{
		return 0;
	}

	return retval;
}

INT shin::CHTTPPost::Recv(SOCKET connectSocket, CHAR* pOutRecvBuf, INT recvBufSize, INT* pOutRecvedDataSize)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// Initialization
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	CHAR* pRecvBuffer = new CHAR[recvBufSize + 1]; // NULL 문자 공간을 위해 + 1
	CHAR* pContent = nullptr;
	CHAR* pCode = nullptr;

	INT recvedSize = 0;
	INT result = 0;
	INT code = 0;

	bool bHeaderFlag = TRUE;
	INT headerLen = 0;
	INT contentLen = 0;

	INT contentRecv = 0;

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// HTTP POST Response 받기
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	for (;;)
	{
		INT retval = recv(connectSocket, pRecvBuffer + recvedSize, recvBufSize - recvedSize, 0);
		if (retval == FIN)
		{
			break;
		}

		if (retval == SOCKET_ERROR)
		{
			return FUNCTION_ERROR;
		}

		recvedSize += retval;

		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		// HTTP Header 획득
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		if (bHeaderFlag == TRUE)
		{
			// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
			// 첫 0x20을 찾아서 Status code 획득
			// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
			pContent = strchr(pRecvBuffer, 0x20);
			if (pContent != nullptr)
			{
				pCode = strchr(pContent + 1, 0x20);
				if (pCode != nullptr)
				{
					// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
					// Status code를 획득하기 위해서 Status code 뒤에 잠시 NULL 문자 삽입
					// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
					*pCode = '\0';
					code = atoi(pContent + 1);
					*pCode = 0x20;
				}
			}

			// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
			// Content-Legth 획득
			// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
			pContent = strstr(pRecvBuffer, "Content-Length:");
			if (pContent != nullptr)
			{
				pCode = strchr(pContent + _countof("Content-Length:") - 1, 0x0d);
				if (pCode != nullptr)
				{
					// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
					// Content-Length 획득하기 위해서 Content-Length: 뒤에 잠시 NULL 문자 삽입
					// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
					*pCode = '\0';
					contentLen = atoi(pContent + _countof("Content-Length:") - 1);
					*pCode = 0x0d;
				}
			}

			// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
			// HTTP Header 끝까지 포인터 이동
			// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
			pContent = strstr(pRecvBuffer, "\r\n\r\n");
			if (pContent != nullptr)
			{
				pContent += _countof("\r\n\r\n") - 1;

				// Header size 계산
				headerLen = (INT)(pContent - pRecvBuffer);

				// Header 처리 종료
				bHeaderFlag = FALSE;
			}
		}

		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		// BODY에 있는 모든 데이터 수신
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
		if (recvedSize >= headerLen + contentLen)
		{
			break;
		}
	}

	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// BODY에 데이터가 있다면 데이터 복사
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	if (pOutRecvBuf != nullptr && pContent != nullptr)
	{
		memcpy(pOutRecvBuf, pContent, contentLen);
		pOutRecvBuf[contentLen] = '\0';
	}

	*pOutRecvedDataSize = contentLen;

	delete[] pRecvBuffer;
	return code;
}

void shin::CHTTPPost::DomainToIP(WCHAR* pHost, IN_ADDR* pServerInAddr)
{
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	// 도메인에서 IP 추출
	//
	// 도메인이 아니라도 pServerInAddr에 정보가 세팅된다.
	// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
	ADDRINFOW hints;
	ADDRINFOW* pServerInfo;

	ZeroMemory(&hints, sizeof(addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	GetAddrInfoW(pHost, NULL, &hints, &pServerInfo);

	*pServerInAddr = ((SOCKADDR_IN*)pServerInfo->ai_addr)->sin_addr;

	FreeAddrInfoW(pServerInfo);
}

void shin::CHTTPPost::UTF16toUTF8(CHAR* pDestnation, WCHAR* pSource, INT len)
{
	WideCharToMultiByte(CP_ACP, 0, pSource, -1, pDestnation, len, NULL, NULL);
}

