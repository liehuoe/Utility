#include "stdafx.h"
#include "MyHttp.h"
#include <wincrypt.h>

using namespace std;

BOOL CHttpSession::SendRequest(LPCWSTR szURL, LPCWSTR szType, LPCSTR szOption, string &strHtml)
{
	if(!m_hSession) return FALSE;

	wstring strHost, strPath;
	WORD wPort;
	AnalyzeURL(szURL, strHost, wPort, strPath);

	BOOL  bResults = FALSE;
	HINTERNET
		hConnect = NULL,
		hRequest = NULL;
	do 
	{
		hConnect=WinHttpConnect(m_hSession, strHost.c_str(), wPort,0);
		if(!hConnect) break;

		hRequest=WinHttpOpenRequest(hConnect, szType, strPath.c_str(), L"HTTP/1.1", WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
		if(!hRequest) break;

		wstring strHeader;
			
// 		strHeader = L"Origin: https://account.xiaomi.com";
// 		WinHttpAddRequestHeaders(hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);
		strHeader = L"Content-type: application/x-www-form-urlencoded";
		WinHttpAddRequestHeaders(hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);
// 		strHeader = L"Accept: */*";
// 		WinHttpAddRequestHeaders(hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);
// 		strHeader = L"Referer: https://account.xiaomi.com/pass/serviceLogin";
// 		WinHttpAddRequestHeaders(hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);
// 		strHeader = L"Accept-Encoding: gzip, deflate";
// 		WinHttpAddRequestHeaders(hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);
// 		strHeader = L"Accept-Language: zh-CN,zh;q=0.8,en;q=0.6";
// 		WinHttpAddRequestHeaders(hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);

		bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)szOption, szOption ? strlen(szOption) : 0, szOption ? strlen(szOption) : 0, 0 );
		if(!bResults) break;

		bResults=WinHttpReceiveResponse(hRequest,NULL);
		if(!bResults) break;
		
		DWORD dwSize = 0;
		strHtml = "";
		do 
        {
			
            // Check for available data.
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                printf( "Error %u in WinHttpQueryDataAvailable.\n",
                GetLastError());
			
            // Allocate space for the buffer.
			LPSTR lpOutBuffer;
            lpOutBuffer = new char[dwSize+1];
            if (!lpOutBuffer)
            {
                printf("Out of memory\n");
                dwSize=0;
            }
            else
            {
                // Read the Data.
                ZeroMemory(lpOutBuffer, dwSize+1);
					
				DWORD dwDownloaded = 0;
                if (!WinHttpReadData( hRequest, (LPVOID)lpOutBuffer, 
                    dwSize, &dwDownloaded))
                {
                    printf( "Error %u in WinHttpReadData.\n", 
                        GetLastError());
                }
                else
                {
                    strHtml += string(lpOutBuffer, dwDownloaded);
					//printf(str.c_str());
                }
				
                // Free the memory allocated to the buffer.
                delete [] lpOutBuffer;
            }
			
        } while (dwSize>0);

		//result = true;
	} while (0);

	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);

	return bResults;
}

void CHttpSession::AnalyzeURL(LPCWSTR szURL, wstring &strHost, WORD &wPort, wstring &strPath)
{
	if(!szURL) return;
	wstring strURL(szURL);
	if(strURL.empty()) return;

	wstring strSeparate;
	
	strSeparate = L"://";
	int nHostIndex = strURL.find(strSeparate);
	if(nHostIndex == wstring::npos)
	{
		nHostIndex = 0;
	}
	else
	{
		nHostIndex += strSeparate.length();
	}
	
	strSeparate = L"/";
	int nPathIndex = strURL.find(strSeparate, nHostIndex);
	if(nPathIndex  == wstring::npos)
	{
		strPath = L"";
		strHost = strURL.substr(nHostIndex);
	}
	else
	{
		strPath = strURL.substr(nPathIndex);
		strHost = strURL.substr(nHostIndex, nPathIndex - nHostIndex);
	}
	
	strSeparate = L":";
	int nPortIndex = strHost.find(strSeparate);
	if(nPortIndex != wstring::npos)
	{
		wPort = wcstoul(strHost.substr(nPortIndex + strSeparate.length()).c_str(), NULL, 10);
	}
	strHost = strHost.substr(0, nPortIndex);
}

CHttpRequest CHttpSession::NewRequest(LPCWSTR szURL, LPCWSTR szType, DWORD dwFlags)
{
	if(!m_hSession) return CHttpRequest();
	
	wstring strHost, strPath;
	INTERNET_PORT wPort = 80;
	if(dwFlags & WINHTTP_FLAG_SECURE) wPort = 443;
	AnalyzeURL(szURL, strHost, wPort, strPath);

	HINTERNET hConnect, hRequest;
	do 
	{
		hConnect=WinHttpConnect(m_hSession, strHost.c_str(), wPort,0);
		if(!hConnect) break;
		hRequest = WinHttpOpenRequest(hConnect, szType, strPath.c_str(), L"HTTP/1.1", WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, dwFlags);
		if(!hRequest) break;

		return CHttpRequest(hConnect, hRequest);
	} while (0);

	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);

	return CHttpRequest();
}

BOOL CHttpRequest::AddHeader(LPCWSTR szKey, LPCWSTR szValue)
{
	if(!m_hConnect || !m_hRequest) return FALSE;

	wstring strHeader = szKey;
	strHeader += L":";
	strHeader += szValue;

	return WinHttpAddRequestHeaders(m_hRequest, strHeader.c_str(), strHeader.length(), WINHTTP_ADDREQ_FLAG_ADD);
}

BOOL CHttpRequest::Send(LPCSTR szData/* = NULL*/)
{
	if(!m_hConnect || !m_hRequest) return FALSE;

	return WinHttpSendRequest(m_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)szData, szData ? strlen(szData) : 0, szData ? strlen(szData) : 0, 0 );
}

wstring CHttpRequest::GetResponseHeader(LPCWSTR szKey, LPCWSTR szDefaultValue/* = NULL*/)
{
	wstring strValue;
	if(szDefaultValue) strValue = szDefaultValue;
	if(!szKey) return strValue;
	
	if(!m_hConnect || !m_hRequest) return strValue;
	
	if(!WinHttpReceiveResponse(m_hRequest, NULL))
	{
		return strValue;
	}

	LPVOID szBuffer = NULL;
	DWORD dwSize = 0;

	WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CUSTOM, szKey, WINHTTP_NO_OUTPUT_BUFFER, &dwSize, WINHTTP_NO_HEADER_INDEX);
	if(dwSize <= 0)
	{
		return strValue;
	}

	szBuffer = new BYTE[dwSize];
	if(!szBuffer) return strValue;
	BOOL ret = WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_CUSTOM, szKey, szBuffer, &dwSize, WINHTTP_NO_HEADER_INDEX);
	if(ret)
	{
		strValue.assign((LPCWSTR)szBuffer, dwSize);
	}
	delete[] szBuffer;

	return strValue;
}

string CHttpRequest::GetResponse()
{
	string strText;

	if(!m_hConnect || !m_hRequest) return strText;

	if(!WinHttpReceiveResponse(m_hRequest, NULL))
	{
		return strText;
	}
	
	DWORD dwSize;
	do 
	{
		// Check for available data.
		dwSize = 0;
		if (!WinHttpQueryDataAvailable(m_hRequest, &dwSize))
		{
			printf( "Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
		}
		
		// Allocate space for the buffer.
		LPSTR lpOutBuffer;
		lpOutBuffer = new char[dwSize+1];
		if (!lpOutBuffer)
		{
			printf("Out of memory\n");
			dwSize=0;
		}
		else
		{
			// Read the Data.
			ZeroMemory(lpOutBuffer, dwSize+1);
			
			DWORD dwDownloaded = 0;
			if (!WinHttpReadData( m_hRequest, (LPVOID)lpOutBuffer, dwSize, &dwDownloaded))
			{
				printf( "Error %u in WinHttpReadData.\n", GetLastError());
			}
			else
			{
				strText += string(lpOutBuffer, dwDownloaded);
			}
			
			// Free the memory allocated to the buffer.
			delete [] lpOutBuffer;
		}
		
	} while (dwSize>0);

	return strText;
}

string CHttpSession::UrlEncode(string &strText)
{
	std::string result;
	
	bool bFlag = false;
	for(unsigned int i = 0; i< static_cast<unsigned int>(strText.length()); i++)
	{
		char ch = strText[i];
		if(ch == ' ')
		{
			result += '+';
		}else if(ch >= 'A' && ch <= 'Z'){
			result += ch;
		}else if(ch >= 'a' && ch <= 'z'){
			result += ch;
		}else if(ch >= '0' && ch <= '9'){
			result += ch;
		}else if(ch == '_' || ch == '-' || ch == '-' || ch == '.' || ch == '!' || ch == '~' || ch == '*' || ch == '\'' || ch == '(' || ch == ')' ){
			result += ch;
		}else if(ch == '\r')
		{
			continue;
		}else if(ch == '\n')
		{
			result += '&';
			bFlag = false;
		}else if(ch == ':' && !bFlag)
		{
			result += '=';
			bFlag = true;
		}else{
			result += '%';
			char szHex[5];
			sprintf(szHex, "%02X", (BYTE)ch);
			result += szHex;
		}
	}
	return result;
}

string CHttpSession::Unicode2Utf8(LPCWSTR szUnicode)
{  
	string strResult;
	if(!szUnicode) return strResult;
	
	int len = WideCharToMultiByte(CP_UTF8, 0, szUnicode, -1, NULL, 0, NULL, NULL);  
	
	strResult.resize(len + 1);
	WideCharToMultiByte(CP_UTF8, 0, szUnicode, -1, &strResult[0], len, NULL,NULL);
	
	return strResult;
}