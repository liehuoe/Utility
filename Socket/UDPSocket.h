#if !defined(_UDP_SOCKET_H_)
#define _UDP_SOCKET_H_

#if _MSC_VER > 1000
#pragma once
#endif 

#include "winsock2.h"
#pragma comment(lib,"ws2_32.lib")

class UDPSocket
{
public:
	UDPSocket()
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		
		m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}
	~UDPSocket()
	{
		if(m_socket) closesocket(m_socket);
		m_socket = NULL;

		WSACleanup();
	}

	BOOL Bind(u_short usPort);
	BOOL Listen(/*func*/);
	BOOL Send(LPCSTR szIP, u_short usPort, const char *data, int len);
	BOOL Send(u_long ulIP, u_short usPort, const char *data, int len);
private:
	SOCKET m_socket;

	static void RecvThread(LPVOID lParam);
};

#endif