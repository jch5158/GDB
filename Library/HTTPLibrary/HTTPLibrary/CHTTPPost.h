#pragma once

// WinSock
#pragma comment(lib, "ws2_32.lib")
#include<WinSock2.h>
#include<WS2tcpip.h>

// Windows
#include<Windows.h>
#include <tchar.h>

// C++
#include<iostream>
#include<string>

// SHIN  Library
#include "library/SystemLog/CSystemLog.h"

namespace shin
{
	constexpr INT FIN = 0;
	constexpr INT HTTP_STATUS_CODE_OK = 200;
	constexpr INT FUNCTION_ERROR = -1;
	constexpr INT MAX_REQUEST_MSG_SIZE = 1024;
	constexpr INT MAX_URL_SIZE = 2048;
	constexpr INT MAX_URI_SIZE = 32;

	// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 
	// ## CHTTPPost __ver 1.0.0
	//
	// #### Description 📝
	// 1. HTTP POST Request
	//
	// #### Warning ❗
	// 1. Timeout 세팅시 입력 단위 조심
	//		
	// #### Useage
	// 1. CHTTPPost(1, 1000, 1000);			         : Timeout 설정
	// 2. http.SetServerURL(L"http://127.0.0.1/");   : 연결할 웹 서버 설정
	// 3. http.POST(…);								 : POST
	//
	// #### Update 😁
	// 1. NEW
	//
	// #### Last Modified
	// 2021 - 2 - 9
	// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ 
	class CHTTPPost
	{
	public:
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// CHTTPPost _ Constructor
		//
		// Parameters ——————————————————————————————————————————————
		//
		// connectTimeout : sec 단위로 입력
		// recvTimeout	  : ms  단위로 입력
		// sendTimeout	  : ms  단위로 입력
		//
		// Return     ––––––––––––––––––––––––––––––––––––––––––––––
		// 
		// NONE
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		CHTTPPost(INT connectTimeout, INT recvTimeout, INT sendTimeout);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// CHTTPPost _ Destructor
		//
		// Parameters ––––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		//
		// Return     ––––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		~CHTTPPost(void);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// POST
		//
		// Parameters ––––––––––––––––––––––––––––––––––––––––––––––
		//
		// pURI                : URI
		// pPostData           : 전송할 데이터 버퍼 포인터
		// pOutRecvBuf         : 응답받을 버퍼 포인터
		// recvBufSize         : 응답받을 버퍼 크기
		// pOutRecvedDataSize  : 응답받은 데이터 크기
		//  
		// Return     ––––––––––––––––––––––––––––––––––––––––––––––
		//
		// HTTP_STATUS_CODE_OK : 응답 성공
		// FUNCTION_ERROR      : 함수 오류
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		INT POST(const CHAR* pURI, const CHAR* pPostData, CHAR* pOutRecvBuf, INT recvBufSize, INT* pOutRecvedDataSize);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// SetServerURL
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// URL        : 연결할 서버 URL
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		void SetServerURL(std::wstring URL);

	private:
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// InitNetwork
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// TRUE       : 함수 성공
		// FALSE      : 함수 실패
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		bool InitNetwork(void);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// Connect
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// (SOCKET)       : Web Server와 연결된 socket
		// INVALID_SOCKET : 연결실패
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		SOCKET Connect(void);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// Send
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// connectSocket : 전송대상 socket
		// requestMsg    : 요청 메시지 포인터
		// pPostData     : 전송할 데이터 버퍼 포인터
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// (INT)		 : 전송한 데이터 크기
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		INT Send(SOCKET connectSocket, CHAR* requestMsg, const CHAR* pPostData);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// Recv
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// connectSocket      : 수신대상 socket
		// pOutRecvBuf        : 데이터 수신 받을 버퍼 포인터
		// recvBufSize        : 수신 받을 버퍼 크기
		// pOutRecvedDataSize : 수신 받은 데이터 크기를 세팅받을 변수 포인터
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// (INT)              : Status code
		// FIN				  : 서버에서 연결 종료
		// FUNCTION_ERROR     : 함수 오류
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		INT Recv(SOCKET connectSocket, CHAR* pOutRecvBuf, INT recvBufSize, INT* pOutRecvedDataSize);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// DomainToIP
		//
		// 도메인이 아닌 IP를 넣어도 상관없다. 
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// pHost	     : 변환할 도메인 주소
		// pServerInAddr : IP에 대한 정보를 세팅받을 IN_ADDR 포인터
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		void DomainToIP(WCHAR* pHost, IN_ADDR* pServerInAddr);


		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◥
		// UTF16toUTF8
		//
		// Parameters –––––––––––––––––––––––––––––––––––––––––––––
		//
		// pDestnation : UTF-8로 변환된 문자를 저장할 버퍼 포인터
		// pSource     : UTF-8로 변환할 UTF-16 문자 버퍼 포인터
		// len		   : UTF-8 문자를 저장할 버퍼의 크기
		//
		// Return     –––––––––––––––––––––––––––––––––––––––––––––
		//
		// NONE
		// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––◢
		void UTF16toUTF8(CHAR* pDestnation, WCHAR* pSource, INT len);

	private:
		SOCKET mConnectSocket;

		INT mRecvTimeout;
		INT mSendTimeout;
		INT mConnectTimeout;

		CHAR mURI[64];
		CHAR mHost[64];
		WORD mPort;

		SOCKADDR_IN mServerSockAddr;
	};
}

