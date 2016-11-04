// snmptest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "snmptest.h"
#include <stdlib.h>  
#include <string.h>  
#include <stdio.h> 
#include<string.h>
#include <windows.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;
#include "winsock2.h"
#include<string.h>

#pragma comment(lib, "ws2_32.lib")


typedef struct tagSENDDATA
{
	CString strOid; //oid
	byte    sOidType; //oid类型 1--整型 2--字符串 3--IP地址
	short   sBytes; //数据长度(字节)
	union{
		char sVal[1024];
		DWORD uIPaddress;  
		int   nValue;
	}value;
}SENDDATA,*PSENDDATA;

typedef struct tagRECVDATA
{
	CString strOid; //oid
	byte    sOidType; //oid类型 1--整型 2--字符串 3--IP地址
	short   sBytes; //数据长度(字节)
	union{
		char sVal[1024];
		DWORD uIPaddress;  
		int   nValue;
	}value;
}RECVDATA,*PRECVDATA;

typedef struct tagSNMPPACKET
{
	short sPackLen;
	char  Buf[4096];
}SNMPPACKET,*PSNMPPACKET;
CString IDToString(DWORD dwRequestId)
{
	CString str;
	
	str.Format("%X",dwRequestId);
	
	if ( str.GetLength() % 2 != 0 )
	{
		str.Insert(0,"0");
	}
	return str;
}

//把字符串的OID转换为十六进制
CString OidConvertHex(CString strOid)
{
	DWORD dwOid;
	byte byVal;
	bool bFlag = true;
	CString strValue = "2b",str,strTemp;
	strOid.Delete(0,4);
	int nIndex = strOid.Find(".",0);
	while ( bFlag )
	{	
		
		if ( nIndex != -1 )
		{
			str = strOid.Mid(0,nIndex);
		}
		else
		{
			str = strOid;
			bFlag = false;
		}
		dwOid = atoi(str);
		//	printf("%d\n",dwOid);
		if ( dwOid >= 128 )
		{
			byVal = (byte)(dwOid % 128);
			str.Format("%02X",byVal);
			strTemp = str;
			
			while ( dwOid >= 128 )
			{
				dwOid = dwOid - byVal;
				dwOid = dwOid / 128;
				byVal = (byte)(dwOid % 128);
				str.Format("%02X",byVal+0x80);
				strTemp.Insert(0,str);
			}
		//	printf("%s\n",strTemp);
			strValue += strTemp;

		}
		else
		{
			str.Format("%02X",dwOid);
			strValue += str;
		}
		strOid.Delete(0,nIndex+1);
		nIndex = strOid.Find(".",0);
	}
	
	return strValue;
}
CString CummityStringToHexString(CString strCumity)
{
	int nLen;
	CString str,strValue;
	char chBuf[20];
	memset(chBuf,0,20);
	strcpy(chBuf,strCumity);
	nLen = strCumity.GetLength();
	for ( int i=0; i<nLen; i++)
	{
		str.Format("%02X",chBuf[i]);
		strValue += str;
	}
	return strValue;
}
int HexToDec(char *buf,CString strHex)
{
	strHex.Replace(" ","");
	int nCount = strHex.GetLength() / 2;
	CString str;
	for ( int i=0; i<nCount; i++ )
	{
		str = strHex.Mid(i*2,2);
		buf[i] = (char)strtol(str,NULL,16);;
	}
	
	return nCount;
}
void Hex2Str( const char *sSrc,  char *sDest, int nSrcLen )  
{  
    int  i;  
    char szTmp[3];  
  
    for( i = 0; i < nSrcLen; i++ )  
    {  
        sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );  
        memcpy( &sDest[i * 2], szTmp, 2 );  
    }  
    return ;  
} 

//创建发送数据包时，从最后字节开始填充，具体解析可以根据实际snmp包来分析
bool CreateGetPacket(LPCSTR strOid[],int nOidNum,CString strReadCumity,
									 DWORD dwRequestId,PSNMPPACKET pSnmpPacket)
{
	char m_snmpBuf[500];
	memset(m_snmpBuf,0x00,500);
		CString strValue,str,strSnmpPack,strOidEx;
		int nLen = 0;
			strValue.Empty();
			strSnmpPack.Insert(0,"0500");
	try
	{				

	
		for ( int i = nOidNum-1; i>= 0; i-- )
		{

			str = OidConvertHex(strOid[i]);

			strValue.Insert(0,str);
			nLen = str.GetLength() / 2;
			str.Format("%02X",nLen);  
			strValue.Insert(0,str);	
			strValue.Insert(0,"06");
		
			nLen = strValue.GetLength() / 2;
			str.Format("%04X",nLen);
			strValue.Insert(0,str);  
			strValue.Insert(0,"3082");	

			strSnmpPack.Insert(0,strValue);

		}

		nLen = strSnmpPack.GetLength() / 2;
		str.Format("%04X",nLen);
		strSnmpPack.Insert(0,str);
		strSnmpPack.Insert(0,"3082");
		
		strSnmpPack.Insert(0,"020100"); //error index
		strSnmpPack.Insert(0,"020100"); //error status


		str = IDToString(dwRequestId);

		strSnmpPack.Insert(0,str);
		


		nLen = str.GetLength() / 2;
		str.Format("%02X",nLen);
		strSnmpPack.Insert(0,str);
		strSnmpPack.Insert(0,"02");

	


		nLen = strSnmpPack.GetLength() / 2;
		str.Format("%04X",nLen);
		strSnmpPack.Insert(0,str);
		strSnmpPack.Insert(0,"a082");

		str = CummityStringToHexString(strReadCumity);

		strSnmpPack.Insert(0,str);


		nLen = str.GetLength() / 2;
		str.Format("%02X",nLen);
		strSnmpPack.Insert(0,str);
		strSnmpPack.Insert(0,"04");

		strSnmpPack.Insert(0,"020100"); //version

		nLen = strSnmpPack.GetLength() / 2;
		str.Format("%04X",nLen);
		strSnmpPack.Insert(0,str);
		strSnmpPack.Insert(0,"3082");

		int nSize = HexToDec(m_snmpBuf,strSnmpPack);
	//	Hex2Str(strSnmpPack,m_snmpBuf,strlen(strSnmpPack));
	//	nSize = strlen(m_snmpBuf);
		
		if ( pSnmpPacket )
		{
			pSnmpPacket->sPackLen = nSize;
			memcpy(pSnmpPacket->Buf,m_snmpBuf,nSize);
//			return true;
		}
//		return false;
	}
	catch (...)
	{
//		return false;
	}
	return false;
}


void main()
{
   int s,len;  
    struct sockaddr_in addr;  
    int addr_len =sizeof(struct sockaddr_in);  
   char buffer[256];  
   char m_snmpBuf[256];
   
   /* 建立socket*/  

   // Initialize Winsock.
 WSADATA wsaData;
 int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
 if (iResult != NO_ERROR)
 printf("Error at WSAStartup()\n");


   if((s = socket(AF_INET,SOCK_DGRAM,0))<0)
   {  
        printf("socket\n");  
       exit(1);  
    }  
   /* 填写sockaddr_in*/  
 //   bzero(&addr,sizeof(addr)); 
   memset((char *)&addr,0,sizeof(addr));
   addr.sin_family = AF_INET;  
    addr.sin_port = htons(1111);  
    addr.sin_addr.s_addr = inet_addr("192.168.1.137");  
 //  while(1){  
//       bzero(buffer,sizeof(buffer));  
	//   strcpy(buffer,"1.3.6.1.4.1.16985.1.12.2.5.1.1.13");
       /* 从标准输入设备取得字符串*/  
    //    len =read(STDIN_FILENO,buffer,sizeof(buffer));  
       /* 将字符串传送给server端*/  
	   SNMPPACKET pSnmpPack;
		long dwID = 1;
		LPCSTR lpszOid[17];
		lpszOid[1] = "1.3.6.1.4.1.16985.1.9.10.1.137";//16985
	//	1.3.6.1.4.1.16985.1.9.1.1.7
	//		1.3.6.1.4.1.16985.1.9.10.1.8
		SENDDATA sendData[2];

		sendData[0].strOid.Format("1.3.6.1.4.1.16985.1.9.10.1.137");
		sendData[0].sOidType = 1;
		sendData[0].value.nValue = 137;

		sendData[1].strOid.Format("1.3.6.1.4.1.16985.1.9.2.1");
		sendData[1].sOidType = 2;
		sendData[1].sBytes = 1224;
		memset(sendData[1].value.sVal,0,1224);
	//	memcpy(sendData[1].value.sVal,m_strCheckTime,14);
	//	sendData[1].value.sVal[14] = '\0';


//		lpszOid[0] = "1.3.6.1.4.1.16985.1.9.2.1";
//char lpszOidww[] =  "1.3.6.1.4.1.16985.1.9.10.1.137";
CString str = OidConvertHex(lpszOidww);
		int nSize = HexToDec(m_snmpBuf,str);

		if ( pSnmpPack.Buf )
		{
			pSnmpPack.sPackLen = nSize;
			memcpy(pSnmpPack.Buf,m_snmpBuf,nSize);

		}

	 //  CreateGetPacket(lpszOid,2,"GLBLTECH",dwID,&pSnmpPack);
	 //  for(int i = 0 ;i < 10; i++)
	 //  {
		   if(pSnmpPack.Buf)
		   {
			   
#if 1
			  {
				int i;
				printf("\nRequest: \n");
				for (i = 0 ; i < pSnmpPack.sPackLen ; i++) 
				{
				  if ((i % 16) == 0) printf("\n  %04x : ", i);
				  printf("%02x ", (unsigned char)pSnmpPack.Buf[i]);
				}
				printf("\nlen:%d\n",i);
				printf("\n\n");
			  }
#endif

			sendto(s,(const char *)(pSnmpPack.Buf),pSnmpPack.sPackLen,0,(struct sockaddr *)&addr,addr_len);  
        /* 接收server端返回的字符串*/  
			len = recvfrom(s,buffer,sizeof(buffer),0,(struct sockaddr *)&addr,&addr_len);  
			if(len < 0)
			{
				printf("recv err\n");
			}
			else
			{

#if 1
			  {
				int i;
				printf("\nResponse: \n");
				for (i = 0 ; i < len ; i++) 
				{
				  if ((i % 16) == 0) printf("\n  %04x : ", i);
				  printf("%02x ", (unsigned char)buffer[i]);
				}
				printf("\n\n");
			  }
#endif
			}
		   }
	//	   Sleep(500);
	//   }


 //  } 

		closesocket(s);

}
/*
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
		CString strHello;
		strHello.LoadString(IDS_HELLO);
		cout << (LPCTSTR)strHello << endl;
	}

	return nRetCode;
}*/


