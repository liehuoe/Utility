#include "HttpClient.h"

using namespace std;

void HttpClient::Release()
{
	if (m_hRequest) { WinHttpCloseHandle(m_hRequest); m_hRequest = NULL; }
	if (m_hConnect) { WinHttpCloseHandle(m_hConnect); m_hConnect = NULL; }
}

BOOL HttpClient::SetRequest(LPCWSTR szURL, REQUEST_TYPE type)
{
	if(!szURL) return FALSE;
	
	wstring strURL = szURL;
	if(strURL.find(L"//") == wstring::npos) strURL.insert(0, L"http://");

	URL_COMPONENTS urlCom = {0};
	urlCom.dwStructSize = sizeof(urlCom);
	WCHAR szHostName[1024] = {0};
	urlCom.lpszHostName = szHostName;
	urlCom.dwHostNameLength = sizeof(szHostName) / sizeof(WCHAR);
	WCHAR szUrlPath[1024] = {0};
	urlCom.lpszUrlPath = szUrlPath;
	urlCom.dwUrlPathLength = sizeof(szUrlPath) / sizeof(WCHAR);
	if(!WinHttpCrackUrl(strURL.c_str(), strURL.length(), ICU_ESCAPE, &urlCom)) return FALSE;

	HINTERNET hConnect = NULL, hRequest = NULL;
	do 
	{
		if(!m_hSession && !(m_hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0))) break;
		if(!(hConnect = WinHttpConnect(m_hSession, urlCom.lpszHostName, urlCom.nPort, 0))) break;
		if(!(hRequest = WinHttpOpenRequest(hConnect, type == REQUEST_TYPE_POST ? L"POST" : L"GET", urlCom.lpszUrlPath, NULL, WINHTTP_NO_REFERER, 
			WINHTTP_DEFAULT_ACCEPT_TYPES, urlCom.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0))) break;
	} while (0);

	if(hConnect && hRequest)
	{
		Release();
		m_hConnect = hConnect;
		m_hRequest = hRequest;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL HttpClient::AddHeader(LPCWSTR szHeader)
{
	if(!szHeader) return FALSE;
	return WinHttpAddRequestHeaders(m_hRequest, szHeader, wcslen(szHeader), WINHTTP_ADDREQ_FLAG_ADD);
}

BOOL HttpClient::Send(LPCSTR szPostData, int len)
{
	if(!szPostData)
		return WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, 0, 0, 0, 0);
	else
	{
		if(len <= 0) len = strlen(szPostData);
		return WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)szPostData, len, len, 0);
	}
}

string HttpClient::Recv()
{
	string strText;
	
	if(!m_hConnect || !m_hRequest) return strText;
	if(!WinHttpReceiveResponse(m_hRequest, NULL)) return strText;
	
	DWORD dwSize;
	do 
	{
		// Check for available data.
		dwSize = 0;
		WinHttpQueryDataAvailable(m_hRequest, &dwSize);		
		// Allocate space for the buffer.
		LPSTR pBuf = new char[dwSize + 1];
		if (!pBuf) dwSize = 0; //Out of memory
		else
		{
			// Read the Data.
			ZeroMemory(pBuf, dwSize + 1);
			
			DWORD dwDownloaded = 0;
			if (WinHttpReadData(m_hRequest, (LPVOID)pBuf, dwSize, &dwDownloaded))
			{
				strText += string(pBuf, dwDownloaded);
			}
			
			// Free the memory allocated to the buffer.
			delete[] pBuf;
		}
	} while(dwSize > 0);
	
	return strText;
}
