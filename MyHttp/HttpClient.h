/*
ʹ��˵��:SetRequest->AddHeader(��ѡ)->Send->Recv
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
	HttpClient() { m_hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0); }
	~HttpClient() { Release(); if(m_hSession) WinHttpCloseHandle(m_hSession); }

	//����Ҫ���������URL
	BOOL SetRequest(LPCWSTR szURL, REQUEST_TYPE type = REQUEST_TYPE_GET);
	//����requestͷ��
	BOOL AddHeader(LPCWSTR szHeader);
	//����request����, szPostData,len:Ҫ������POST���ݺͳ���,szPostData��ʾ����������,len=-1��ʾszPostDataΪNULL�ֽڽ�β��C�ַ���
	BOOL Send(LPCSTR szPostData = NULL, int len = -1);
	std::string Recv();
private:
	void Release();
	HINTERNET m_hSession;
	HINTERNET m_hConnect;
	HINTERNET m_hRequest;
};

#endif