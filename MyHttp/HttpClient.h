/*
使用说明:SetRequest->AddHeader(可选)->Send->Recv
*/

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <string>
#include <WINSOCK2.H>
#include "./WinHttp/winhttp.h"

enum REQUEST_TYPE {REQUEST_TYPE_GET, REQUEST_TYPE_POST};

class HttpClient
{
public:
	~HttpClient() { Release(); }

	BOOL SetRequest(LPCWSTR szURL, REQUEST_TYPE type = REQUEST_TYPE_GET);
	BOOL AddHeader(LPCWSTR szHeader);
	BOOL Send(LPCSTR szPostData = NULL, int len = 0);
	std::string Recv();
private:
	void Release();
	HINTERNET m_hSession;
	HINTERNET m_hConnect;
	HINTERNET m_hRequest;
};

#endif