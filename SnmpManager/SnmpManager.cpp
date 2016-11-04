#include "StdAfx.h"
#include "SnmpManager.h"

using namespace std;

smiINT32 CSnmpSession::m_nRequestID = 0;

CSnmpSession::CSnmpSession()
:	m_hSession(NULL), 
	m_hContext(NULL), 
	m_hVariable(NULL) 
{
	InitializeCriticalSection(&m_csRequestID);
	InitializeCriticalSection(&m_csRequests);
}

CSnmpSession::CSnmpSession(CSnmpSession &session)
{ 
	InitializeCriticalSection(&m_csRequestID);
	InitializeCriticalSection(&m_csRequests);

	*this = session; 
}

CSnmpSession::~CSnmpSession()
{
	DeleteCriticalSection(&m_csRequestID);
	DeleteCriticalSection(&m_csRequests);

	Destroy();
}

void CSnmpSession::Destroy()
{
	CleanVariable();
	if(m_hContext) { SnmpFreeContext(m_hContext); m_hContext = NULL; }
	if(m_hSession) { SnmpClose(m_hSession); m_hSession = NULL; }
}

SNMPAPI_STATUS CSnmpSession::SnmpCallback(
	HSNMP_SESSION hSession,
	HWND hWnd,
	UINT wMsg,
	WPARAM wParam,
	LPARAM lParam,
	LPVOID lClientpData
)
{
	CSnmpSession *pThis = (CSnmpSession*)lClientpData;
	if(!pThis) return SNMPAPI_FAILURE;

	smiINT32 nRequestId = (smiINT32)lParam;

	EnterCriticalSection(&pThis->m_csRequests);
	if(pThis->m_requests.find(nRequestId) != pThis->m_requests.end())
	{
		SetEvent(pThis->m_requests[nRequestId]);
	}
	LeaveCriticalSection(&pThis->m_csRequests);

	return SNMPAPI_SUCCESS;
}

BOOL CSnmpSession::Create()
{
	Destroy();

	m_hSession = SnmpCreateSession(NULL, NULL, SnmpCallback, this);
	CreateContext();
	return m_hSession != SNMPAPI_FAILURE;
}

BOOL CSnmpSession::CreateContext()
{
	if(!m_hSession || m_strCommunity.empty()) return FALSE;

	if(m_hContext) SnmpFreeContext(m_hContext);
	
	smiOCTETS com;
	com.ptr = (BYTE*)m_strCommunity.c_str();
	com.len = m_strCommunity.length();
	m_hContext = SnmpStrToContext(m_hSession, &com);//创建上下文句柄,共同体

	return m_hContext != SNMPAPI_FAILURE;
}

BOOL CSnmpSession::SetCommunity(LPCSTR szCommunity)
{
	if(!szCommunity) return FALSE;
	m_strCommunity = szCommunity;
	return CreateContext();
}

BOOL CSnmpSession::AddVariable(LPCSTR szOID, const smiVALUE& value)
{
	if(!m_hSession) return FALSE;

	if(!m_hVariable)
	{
		//创建变量捆绑列表
		if(SNMPAPI_FAILURE == (m_hVariable = SnmpCreateVbl(m_hSession, 0, 0))) return FALSE;
	}

	//追加绑定列表
	smiOID oid;
	if(SNMPAPI_FAILURE != SnmpStrToOid(szOID, &oid))
	{
		BOOL bRet = FALSE;
		if(SNMPAPI_FAILURE != SnmpSetVb(m_hVariable, 0, &oid, &value))
		{
			bRet = TRUE;
		}
		SnmpFreeDescriptor(SNMP_SYNTAX_OID, (smiLPOPAQUE)&oid);
		return bRet;
	}
	else
	{
		return FALSE;
	}
}

BOOL CSnmpSession::SendRequest(smiINT type, LPCSTR szIP, DWORD dwTimeout, DWORD dwRetry)
{
	if(!szIP || !m_hSession || !m_hContext || !m_hVariable) return FALSE;

	BOOL bResult = FALSE;
	HSNMP_ENTITY hSrc = NULL, hDst = NULL;
	HSNMP_PDU hPDU = NULL;
	do 
	{
		//创建源实体
		if(SNMPAPI_FAILURE == (hSrc = SnmpStrToEntity(m_hSession, "127.0.0.1"))) break;
		//创建目标实体
		if(SNMPAPI_FAILURE == (hDst = SnmpStrToEntity(m_hSession, szIP))) break;
		SnmpSetTimeout(hDst, dwTimeout);//设置超时时间
		SnmpSetRetry(hDst, dwRetry);//设置重传次数
		//创建PDU
		EnterCriticalSection(&m_csRequestID);
		smiINT32 nRequestID = ++m_nRequestID;
		LeaveCriticalSection(&m_csRequestID);
		if(SNMPAPI_FAILURE == (hPDU = SnmpCreatePdu(m_hSession, type, nRequestID, 0, 0, m_hVariable))) break;
		//发送消息
		if(SNMPAPI_FAILURE == SnmpSendMsg(m_hSession, hSrc, hDst, m_hContext, hPDU)) break;

		bResult = m_nRequestID;
	} while (0);

	if(hSrc) SnmpFreeEntity(hSrc);
	if(hDst) SnmpFreeEntity(hDst);
	CleanVariable();
	if(hPDU) SnmpFreePdu(hPDU);
	return bResult;
}

BOOL CSnmpSession::GetResponse(SnmpData &data)
{
	data.Reset();

	SNMPAPI_STATUS bResult = SNMPAPI_FAILURE;
	HSNMP_ENTITY hSrc = NULL, hDst = NULL;
	HSNMP_CONTEXT hContext = NULL;
	HSNMP_PDU hPDU = NULL;
	HSNMP_VBL varbindlist = NULL;
	do 
	{
		if(SNMPAPI_FAILURE == SnmpRecvMsg(m_hSession, &hSrc, &hDst, &hContext, &hPDU)) break;

		char szIP[32];
		if(SNMPAPI_FAILURE == SnmpEntityToStr(hSrc, 32, szIP)) break;
		data.deviceIP = szIP;
		
		smiINT PDU_type;
		smiINT32 request_id;
		smiINT error_status;
		smiINT error_index;
		if(SNMPAPI_FAILURE == SnmpGetPduData(hPDU, &PDU_type, &request_id, &error_status, &error_index, &varbindlist)) break;

		data.pduType = PDU_type;

		BOOL bSuccess = TRUE;
		int nCount = SnmpCountVbl(varbindlist);
		for(int i = 0; i < nCount; ++i)
		{
			smiOID name;
			smiVALUE value;
			if(SNMPAPI_FAILURE == SnmpGetVb(varbindlist, i + 1, &name, &value))
			{
				bSuccess = FALSE;
				break;
			}

			char szOID[MAX_PATH];
			if(SNMPAPI_FAILURE == SnmpOidToStr(&name, MAX_PATH, szOID))
			{
				bSuccess = FALSE;
				SnmpFreeDescriptor(SNMP_SYNTAX_OID, (smiLPOPAQUE)&name);
				break;
			}
			data.variables[szOID] = value;

			SnmpFreeDescriptor(SNMP_SYNTAX_OID, (smiLPOPAQUE)&name);
		}
		if(!bSuccess) break;
		
		bResult = SNMPAPI_SUCCESS;
	} while (0);
	
 	if(hSrc) SnmpFreeEntity(hSrc);
 	if(hDst) SnmpFreeEntity(hDst);
 	if(hContext) SnmpFreeContext(hContext);
 	if(hPDU) SnmpFreePdu(hPDU);
	if(varbindlist) SnmpFreeVbl(varbindlist);

	return bResult;
}

BOOL CSnmpSession::GetResponse(smiINT32 nRequestId, SnmpData &data, DWORD dwTimeout, HANDLE hExitSign)
{
	BOOL bRet = FALSE;

	HANDLE hWait = CreateEvent(NULL, TRUE, FALSE, "SnmpManager");
	if(!hWait) return FALSE;

	EnterCriticalSection(&m_csRequests);
	m_requests[nRequestId] = hWait;
	LeaveCriticalSection(&m_csRequests);

	HANDLE hSign[2] = {hWait, hExitSign};
	if(WAIT_OBJECT_0 == WaitForMultipleObjects(hExitSign ? 2 : 1, hSign, FALSE, dwTimeout))
	{
		bRet = GetResponse(data);
	}

	EnterCriticalSection(&m_csRequests);
	m_requests.erase(nRequestId);
	LeaveCriticalSection(&m_csRequests);

	CloseHandle(hWait);
	return bRet;
}
