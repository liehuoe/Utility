#ifndef _MYHTTP_H_
#define _MYHTTP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <string>

class CHttpRequest
{
public:
	CHttpRequest()
	{
		m_hConnect = NULL;
		m_hRequest = NULL;
	}
	CHttpRequest(HINTERNET hConnect, HINTERNET hRequest)
	{
		m_hConnect = hConnect;
		m_hRequest = hRequest;
	}
	~CHttpRequest() { Close(); }
	void Close() 
	{
		if (m_hRequest) WinHttpCloseHandle(m_hRequest);
		if (m_hConnect) WinHttpCloseHandle(m_hConnect);
	}
	operator bool() { return m_hConnect && m_hRequest ? true : false; }
	CHttpRequest& operator=(CHttpRequest &r) 
	{
		Close(); 
		m_hConnect = r.m_hConnect; 
		m_hRequest = r.m_hRequest;
		r.m_hConnect = NULL;
		r.m_hRequest = NULL;
		return *this; 
	}

	BOOL AddHeader(LPCWSTR szKey, LPCWSTR szValue);
	BOOL Send(LPCSTR szData = NULL);
	std::wstring GetResponseHeader(LPCWSTR szKey, LPCWSTR szDefaultValue = NULL);
	std::string GetResponse();
private:
	HINTERNET m_hConnect;
	HINTERNET m_hRequest;
};

class CHttpSession
{
public:
	CHttpSession()
	{
		m_hSession = NULL;
	}
	CHttpSession(HINTERNET hSession)
	{
		m_hSession = hSession;
	}
	~CHttpSession()
	{
		Close();
	}
	static CHttpSession New()
	{
		return CHttpSession(WinHttpOpen(NULL,WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
	}
	void Close() { if(m_hSession) WinHttpCloseHandle(m_hSession); }
	operator bool() { return m_hSession ? true : false; }
	CHttpSession& operator=(CHttpSession &s) 
	{
		Close();
		m_hSession = s.m_hSession;
		s.m_hSession = NULL;
		return *this; 
	}

	CHttpRequest NewHttpGet(LPCWSTR szURL) { return NewRequest(szURL, L"GET", 0); }
	CHttpRequest NewHttpsGet(LPCWSTR szURL) { return NewRequest(szURL, L"GET", WINHTTP_FLAG_SECURE); }
	CHttpRequest NewHttpPost(LPCWSTR szURL) { return NewRequest(szURL, L"POST", 0); }
	CHttpRequest NewHttpsPost(LPCWSTR szURL) { return NewRequest(szURL, L"POST", WINHTTP_FLAG_SECURE); }

	BOOL SendRequest(LPCWSTR szURL, LPCWSTR szType, LPCSTR szOption, std::string &strHtml);
private:
	CHttpRequest NewRequest(LPCWSTR szURL, LPCWSTR szType, DWORD dwFlags);
	void AnalyzeURL(LPCWSTR szURL, std::wstring &strHost, WORD &wPort, std::wstring &strPath);
private:
	HINTERNET m_hSession;

public:
	static std::string UrlEncode(std::string &strText);
	std::string Unicode2Utf8(LPCWSTR szUnicode);
};

#endif