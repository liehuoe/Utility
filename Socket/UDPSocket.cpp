//#include "stdafx.h"
#include "UDPSocket.h"
#include <process.h>

BOOL UDPSocket::Bind(u_short usPort)
{
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(usPort);

	return 0 == bind(m_socket,(SOCKADDR*)&addr,sizeof(SOCKADDR));
}

BOOL UDPSocket::Send(u_long ulIP, u_short usPort, const char *data, int len)
{
	SOCKADDR_IN addr;
	addr.sin_addr.S_un.S_addr = htonl(ulIP);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(usPort);

	return SOCKET_ERROR != sendto(m_socket, data, len, 0, (SOCKADDR*)&addr, sizeof(SOCKADDR));
}

BOOL UDPSocket::Send(LPCSTR szIP, u_short usPort, const char *data, int len)
{
	hostent *hostinfo = gethostbyname(szIP);
	if (!hostinfo || h_errno != 0) return FALSE;

	SOCKADDR_IN addr;
	addr.sin_addr = *(in_addr*)hostinfo->h_addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(usPort);

	return SOCKET_ERROR != sendto(m_socket, data, len, 0, (SOCKADDR*)&addr, sizeof(SOCKADDR));
}

BOOL UDPSocket::Listen(/*func*/)
{
	HANDLE hThread = (HANDLE)_beginthread(RecvThread, 0, this);
	if(!hThread) return FALSE;

	CloseHandle(hThread);
	return TRUE;
}

#include <stdio.h>
void UDPSocket::RecvThread(LPVOID lParam)
{	
	UDPSocket *pThis = (UDPSocket*)lParam;
	while(pThis && pThis->m_socket)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(pThis->m_socket, &fds);

		timeval tv = {1, 0};
		int nRet = select(pThis->m_socket + 1, &fds, NULL, NULL, &tv);
		
		if(nRet < 0) break;
		else if(nRet > 0 && FD_ISSET(pThis->m_socket, &fds))
		{
			SOCKADDR_IN addr;
			int len = sizeof(SOCKADDR);
			char buf[1500] = {0};
			nRet = recvfrom(pThis->m_socket, buf, 1500, 0, (SOCKADDR*)&addr, &len);
			if(nRet == -1)
			{
				//发送到不存在的端口会产生WSAECONNRESET错误
				nRet = WSAGetLastError();
				if(nRet != 0 && nRet != WSAECONNRESET) break;
			}
			else
			{
				printf("收到数据:%s\n", buf);
				//sendto(pThis->m_socket, "OK", 2, 0, (SOCKADDR*)&addr, len);
			}
		}
	}
}