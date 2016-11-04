#if !defined(_SNMP_MANAGER_H_)
#define _SNMP_MANAGER_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsnmp.h>
//#pragma comment(lib, "wsnmp32.lib")
#pragma warning(disable : 4786)
#include <string>
#include <map>

typedef struct tagSnmpData
{
	~tagSnmpData() { Reset(); }
	smiINT32 requestId;
	smiINT pduType;
	std::string deviceIP;
	std::map<std::string, smiVALUE> variables;
	void Reset()
	{
		for(std::map<std::string, smiVALUE>::iterator it = variables.begin(); it != variables.end(); ++it)
		{
			if(it->second.syntax)
				SnmpFreeDescriptor(it->second.syntax, (smiLPOPAQUE)&it->second.value);
		}
		variables.clear();
	}
}SnmpData, *LPSnmpData;

class CSnmpSession
{
public:
	CSnmpSession();
	CSnmpSession(CSnmpSession &session);
	~CSnmpSession();
	CSnmpSession& operator=(CSnmpSession &session)
	{
		m_hSession = session.m_hSession;
		m_hContext = session.m_hContext;
		m_hVariable = session.m_hVariable;
		session.m_hSession = NULL;
		session.m_hContext = NULL;
		session.m_hVariable = NULL;

		return *this;
	}
private:
	HSNMP_SESSION m_hSession;
	HSNMP_CONTEXT m_hContext;
	std::string m_strCommunity;
	HSNMP_VBL m_hVariable;

	static smiINT32 m_nRequestID;
	CRITICAL_SECTION m_csRequestID;
	std::map<smiINT32, HANDLE> m_requests;
	CRITICAL_SECTION m_csRequests;
public:
	static void Init()
	{
		smiUINT32 nMajorVersion,
			nMinorVersion,
			nLevel,
			nTranslateMode,
			nRetransmitMode;
		SnmpStartup(&nMajorVersion, &nMinorVersion, &nLevel, &nTranslateMode, &nRetransmitMode);
		
		SnmpSetTranslateMode(SNMPAPI_UNTRANSLATED_V1);//设置传输模式
		SnmpSetRetransmitMode(SNMPAPI_ON);//设置重传模式
	}
	static void Clean()
	{
		SnmpCleanup();
	}

	BOOL Create();
	void Destroy();
	BOOL SetCommunity(LPCSTR szCommunity);
	BOOL AddVariable(LPCSTR szOID, const smiVALUE& value);
	void CleanVariable() { if(m_hVariable) { SnmpFreeVbl(m_hVariable); m_hVariable = NULL; } }

	BOOL GetRequest(LPCSTR szIP, DWORD dwRetry = 3, DWORD dwTimeout = 2000)
	{ return SendRequest(SNMP_PDU_GET, szIP, dwTimeout, dwRetry); }
	BOOL SetRequest(LPCSTR szIP, DWORD dwRetry = 3, DWORD dwTimeout = 2000)
	{ return SendRequest(SNMP_PDU_SET, szIP, dwTimeout, dwRetry); }
	BOOL GetResponse(SnmpData &data);//异步使用
	BOOL GetResponse(smiINT32 nRequestId, SnmpData &data, DWORD dwTimeout = 5000, HANDLE hExitSign = NULL);
protected:
	BOOL SendRequest(smiINT type, LPCSTR szIP, DWORD dwTimeout, DWORD dwRetry);
	BOOL CreateContext();
protected:
	static SNMPAPI_STATUS CALLBACK SnmpCallback(
		HSNMP_SESSION hSession,
		HWND hWnd,
		UINT wMsg,
		WPARAM wParam,
		LPARAM lParam,
		LPVOID lClientpData
    );
};

#endif