#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include<math.h>
#define PORT 161 
#define SERVER_IP "192.168.1.195"
typedef unsigned long int DWORD;
typedef char byte;

unsigned char errorStatus, errorIndex;
unsigned char buffer[1025];
int len = 0;
typedef struct tagSNMPPACK
{

	DWORD uIpAddress;
	int      nLen;
	char     chBuf[1400];
}SNMPPACK;

typedef struct tagRECVDATA
{
//	CString strOid; //oid
	byte    sOidType;
	short   sBytes;
	union{
		char sVal[1024];
		DWORD uIPaddress;  
		int   nValue;
	}value;
}RECVDATA;
typedef struct tagSNMPPACKET
{
	short sPackLen;
	char  Buf[4096];
}SNMPPACKET,*PSNMPPACKET;
DWORD DataConvert0(short sVal1,short sVal2)
{
	DWORD dwValue = 0;

		dwValue = sVal1;
		dwValue = dwValue << 8;
    	dwValue |= sVal2;


	return dwValue;
}
DWORD DataConvert1(short sVal1,short sVal2,short sVal3)
{

	DWORD dwValue = 0;

		dwValue = sVal1;
		dwValue = dwValue << 8;
		dwValue |= sVal2;
		dwValue = dwValue << 8;
    	dwValue |= sVal3;


	return dwValue;
}
DWORD DataConvert2(short sVal1,short sVal2,short sVal3,short sVal4)
{
	DWORD dwValue = 0;

		dwValue = sVal1;
		dwValue = dwValue << 8;
		dwValue |= sVal2;
		dwValue = dwValue << 8;
		dwValue |= sVal3;
		dwValue = dwValue << 8;
		dwValue |= sVal4;

	return dwValue;
}


void ConvertOid(char *buff,byte *pBuf,byte byLen)
{
	char *str;   
	int idex = 0;
	if ( byLen <= 0 )
	{
		return;
	}
	DWORD dwValue = 0;
	int byCnt = 0,byCnt2 = 0,byCnt3 = 0;

	char strOid[128];


	int nIndex = 0;
					int i;

	#if 1
			  {

				printf("\nRequest: \n");
				for (i = 0 ; i < byLen ; i++) 
				{
				  if ((i % 16) == 0) printf("\n  %04x : ", i);
				  printf("%02x ", (unsigned char)pBuf[i]);
				}
				printf("\nlen:%d\n",i);
				printf("\n\n");
			  }
#endif

	
	if ( pBuf[nIndex] == 0x2b )
	{
		nIndex += 1;
		strcpy(strOid,"1.3");
	}
	byLen -= 1;

	while ( byLen > 0 )
	{

		// char *p="0x11";   

//		idex = (int)strtol(p, &str, 16);//十六进制

		if ( pBuf[nIndex] <= 0x7f )
		{
			printf("%02x \n", (unsigned char)pBuf[nIndex]);
		//	str.Format("%d",pBuf[nIndex]);
			sprintf(strOid,"%s.%d",strOid,pBuf[nIndex]);
			nIndex += 1;
		//	strOid += "." + str;
			byLen--;
		}
		else 
		{
			printf("w:%02x \n", (unsigned char)pBuf[nIndex]);
			dwValue=0;
			byCnt = nIndex;
			nIndex += 1;
			byCnt2 = 1;
			while ( pBuf[nIndex] > 0x7f )//0x7f

			{
				++byCnt2;
				nIndex += 1;
			}
			nIndex = byCnt;
			byCnt3 = byCnt2;
			while ( nIndex < byCnt + byCnt2  )
			{
				dwValue += (pBuf[nIndex] - 0x80) * (DWORD)pow(128,byCnt3);
				--byCnt3;
				++nIndex;
			}
			dwValue += pBuf[nIndex];
			nIndex += 1;
			sprintf(strOid,"%s.%ld",strOid,dwValue);
			printf("strOid1:%s\n",strOid);
		//	str.Format("%d",dwValue);
		//	strOid += "." + str;
			byLen -= ( byCnt2 + 1 );
		}


	}
	strncpy(buff,strOid,strlen(strOid));
	return;
}

RECVDATA recvData[10];
int dwRequestId = 0;
char sOid[128];
//bool AnalysisSnmpPack(SNMPPACK *pUdpSnmp,DWORD &dwRequestId,RECVDATA recvData[],int &nLen)
int  snmptest(char * chBuf,int nLen)
{
		int nOidBytes = 0;
		byte  byOidLen = 0;

	//	int nLen = 0;
 		int m_sIndex = 0;
		int m_sBytesCount = 0;
		int m_sOidIndex = -1;
		if ( !((byte)chBuf[m_sIndex] == 0x30))// && (byte)chBuf[m_sIndex+1] == 0x82) ) //判断头部是否正确
		{

			return -1;
		}

		//判断字符串数量是否正确
		if ( DataConvert0((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3]) != (DWORD)nLen - 4 )
		{

			return -1;
		}
		m_sIndex += 4;
		m_sIndex += 3; 
		if ( (byte)chBuf[m_sIndex] != 0x04 ) 
		{
	
			return -1;
		}
		m_sIndex += (byte)chBuf[m_sIndex+1] + 2;
		m_sIndex += 4; 
		if ( (byte)chBuf[m_sIndex] != 0x02 ) 
		{
	
			return -1;
		}
		if ( (byte)chBuf[m_sIndex+1] == 0x01 )
		{
			dwRequestId = (byte)chBuf[m_sIndex+2];
			m_sIndex += 3;
		}
		else
		{
			dwRequestId = DataConvert0((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3]);
			m_sIndex += 4;
		}
		if ( (byte)chBuf[m_sIndex] != 0x02 || (byte)chBuf[m_sIndex+2] != 0 ) 
		{
			return -1;
		}
		m_sIndex += 3;
		m_sIndex += 3; //30
		m_sBytesCount = (unsigned short)DataConvert0((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3]);
		m_sIndex += 4; 

		while ( m_sBytesCount > 0 ) 
		{
			nOidBytes = (int)DataConvert0((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3]);
			m_sIndex += 4;

			m_sBytesCount -=4;

			m_sBytesCount -= nOidBytes;

	        if ( (byte)chBuf[m_sIndex] != 0x06  )
	        {
	
				return -1;
	        }
			byOidLen = (byte)chBuf[m_sIndex+1];

			m_sIndex += 2;
			nOidBytes -= 2;
			nOidBytes -= byOidLen; 

		//	recvData[++m_sOidIndex].strOid = m_dataConvert.ConvertOid((byte *)&pUdpSnmp->chBuf[m_sIndex],byOidLen);
			ConvertOid(sOid,(byte *)&chBuf[m_sIndex],byOidLen);
			printf("SOID:%s,len:%d\n",sOid,byOidLen);///////////////////////////////////////////////////////////
			m_sIndex += byOidLen; 
			if ( (byte)chBuf[m_sIndex] == 0x05 )//0500---end
			{
				m_sOidIndex -= 1;
				m_sIndex += 2;

				return -1;
			}


			else if ( (byte)chBuf[m_sIndex] == 0x04 )//53
			{
				recvData[m_sOidIndex].sOidType = 2;
				if ( (byte)chBuf[m_sIndex+1] == 0x82 )
				{
					recvData[m_sOidIndex].sBytes = (short)DataConvert0((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3]);
					m_sIndex += 4;
				}
				else if ( (byte)chBuf[m_sIndex+1] == 0x81 )
				{
					recvData[m_sOidIndex].sBytes = (byte)chBuf[m_sIndex+2];
					m_sIndex += 3;
				}
				else
				{
					recvData[m_sOidIndex].sBytes = (byte)chBuf[m_sIndex+1];
					m_sIndex += 2;
				}
				memset(recvData[m_sOidIndex].value.sVal,0,1024);
				memcpy(recvData[m_sOidIndex].value.sVal,&chBuf[m_sIndex],recvData[m_sOidIndex].sBytes);//55
				m_sIndex += recvData[m_sOidIndex].sBytes;
			}
			else if ( (byte)chBuf[m_sIndex] == 0x02 )
			{
				recvData[m_sOidIndex].sOidType = 1;
				recvData[m_sOidIndex].sBytes = chBuf[m_sIndex+1];
				if ( recvData[m_sOidIndex].sBytes == 1 )
				{
					recvData[m_sOidIndex].value.nValue = (int)chBuf[m_sIndex+2];
					m_sIndex += 3;
				}
				else if ( recvData[m_sOidIndex].sBytes == 2 )
				{
					recvData[m_sOidIndex].value.nValue = (int)DataConvert0((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3]);
					m_sIndex += 4;
				}
				else if ( recvData[m_sOidIndex].sBytes == 3 )
				{
					recvData[m_sOidIndex].value.nValue = (int)DataConvert1((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3],
					(byte)chBuf[m_sIndex+4]);
					m_sIndex += 5;
				}
				else if ( recvData[m_sOidIndex].sBytes == 4 )
				{
					recvData[m_sOidIndex].value.nValue = (int)DataConvert2((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3],
					(byte)chBuf[m_sIndex+4],(byte)chBuf[m_sIndex+5]);
					m_sIndex += 6;
				}
			}
			else if ( (byte)chBuf[m_sIndex] == 0x40 )
			{
				recvData[m_sOidIndex].sOidType = 3;
				recvData[m_sOidIndex].sBytes = chBuf[m_sIndex+1];
				recvData[m_sOidIndex].value.uIPaddress = DataConvert2((byte)chBuf[m_sIndex+2],(byte)chBuf[m_sIndex+3],
					(byte)chBuf[m_sIndex+4],(byte)chBuf[m_sIndex+5]);
				m_sIndex += 6;
			}
		}

		if ( m_sBytesCount != 0 )
		{

			return ;
		}

	//	nLen = m_sOidIndex + 1;
		
		return 0;
}
void IDToString(char * strdst,DWORD dwRequestId)
{
//	CString str;
	
//	str.Format("%X",dwRequestId);
	char strtemp[20];
	memset(strtemp,0x00,20);
	sprintf(strtemp,"%X",dwRequestId);
	if(strlen(strtemp)%2 != 0)
	{
		sprintf(strtemp,"%s%X","0",dwRequestId);
	}
	strcpy(strdst,strtemp);
}

//把字符串的OID转换为十六进制
//CString OidConvertHex(CString strOid)
void OidConvertHex(char * strdst,char * strsrc,int nLen)
{
	DWORD dwOid;
	byte byVal;


	char strtotal[512];
	char strtemp[512];
	char *p = NULL;
	char *ptok = NULL;;

	memset(strtotal,0x00,512);
	memset(strtemp,0x00,512);
	
	strcpy(strtemp,"2b");

	strncpy(strtotal,strsrc,nLen);

	p = strtotal;

	ptok = strtok(p+4,".");
	while(ptok)
	{
		dwOid = atoi(ptok);
		if ( dwOid >= 128 )
		{
				byVal = (byte)(dwOid % 128);
				
				sprintf(strtemp,"%s%02X",strtemp,byVal);

				while ( dwOid >= 128 )
				{
					dwOid = dwOid - byVal;
					dwOid = dwOid / 128;
					byVal = (byte)(dwOid % 128);
					sprintf(strtemp,"%s%02X%s",strtemp,byVal+0x80);
				}


		}
		else
		{
				sprintf(strtemp,"%s%02X",strtemp,dwOid);

		}
		ptok = strtok(NULL,".");
	}
	strcpy(strdst,strtemp);
}

void CummityStringToHexString(char *strdst,char* strCumity,int len)
{
	int nLen;
	char chBuf[20];
	char chtemp[20];
	char chtotal[20];
	int i=0;

	memset(chBuf,0,20);
	strncpy(chBuf,strCumity,len);


	for (i=0; i<len; i++)
	{
		if(i ==0)
		{

			sprintf(chtotal,"%02X",chBuf[i]);
		}
		else
		{
			strcpy(chtemp,chtotal);
			sprintf(chtotal,"%s%02X",chtemp,chBuf[i]);

		}

	}

	strcpy(strdst,chtotal);
}
int HexToDec(char *buf,char * strHex,int len)
{
//	strHex.Replace(" ","");
	int i = 0;
	char pt[2];
	char *p;
	char str[512];
//	char stemp[512];

	int nCount = len / 2;
	memset(str,0x00,512);
//	memset(stemp,0x00,512);
	strncpy(str,strHex,len);
	p = str;
	for ( i=0; i<nCount; i++ )
	{
		memset(pt,0x00,2);
		strncpy(pt,p+i*2,2);
	//	printf("buf:%s\n",pt);
	//	str = strHex.Mid(i*2,2);
		buf[i] = (char)strtol(pt,NULL,16);
	//	int i = (int)strtol(p, &str, 16);//十六进制

	}
//	strncpy(buf,stemp,nCount);
	
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

int CreateGetPacket(char * strd,int len,int nOidNum,DWORD dwRequestId,PSNMPPACKET pSnmpPacket)
{
	char m_snmpBuf[500];
	char m_strVa[512];
	char stroid[64];
	int i=0;
	memset(m_snmpBuf,0x00,500);
	memset(m_strVa,0x00,512);

		int nLen = 0;
	//	for (i = nOidNum-1; i>= 0; i-- )
	//	{

			strcpy(m_strVa,"0500");
		//	OidConvertHex(stroid,strd,len);
			strcpy(stroid,"2b06010401818459010C010101");
			sprintf(m_snmpBuf,"%s%s",stroid,m_strVa);
		
printf("buf:%s\n",stroid);

			nLen = strlen(stroid)/2;
			memset(stroid,0x00,64);

			sprintf(stroid,"%02X",nLen);
			memset(m_strVa,0x00,512);
			sprintf(m_strVa,"%s%s%s","06",stroid,m_snmpBuf);

			nLen = strlen(m_strVa)/2;
			memset(stroid,0x00,64);

			sprintf(stroid,"%04X",nLen);
			memset(m_snmpBuf,0x00,500);
			sprintf(m_snmpBuf,"%s%s%s","3082",stroid,m_strVa);
	//	}



		nLen = strlen(m_snmpBuf)/2;
		memset(stroid,0x00,64);
		sprintf(stroid,"%04X",nLen);
		memset(m_strVa,0x00,512);
		sprintf(m_strVa,"%s%s%s%s%s","020100","020100","3082",stroid,m_snmpBuf);
	

		IDToString(stroid,dwRequestId);
		memset(m_snmpBuf,0x00,500);
		sprintf(m_snmpBuf,"%s%s",stroid,m_strVa);



		nLen = strlen(stroid)/2;

		memset(m_strVa,0x00,512);
		sprintf(m_strVa,"%s%02X%s","02",nLen,m_snmpBuf);


		nLen = strlen(m_strVa)/2;
		memset(stroid,0x00,64);
		sprintf(stroid,"%04X",nLen);
		memset(m_snmpBuf,0x00,500);
		sprintf(m_snmpBuf,"%s%s%s","a082",stroid,m_strVa);


		memset(stroid,0x00,64);
		CummityStringToHexString(stroid,"GLBLTECH",8);

		memset(m_strVa,0x00,512);
		sprintf(m_strVa,"%s%s",stroid,m_snmpBuf);


		nLen = strlen(stroid)/2;
		memset(stroid,0x00,64);
		sprintf(stroid,"%02X",nLen);

		memset(m_snmpBuf,0x00,500);
		sprintf(m_snmpBuf,"%s%s%s%s","020100","04",stroid,m_strVa);


		nLen = strlen(m_snmpBuf)/2;
		memset(stroid,0x00,64);
		sprintf(stroid,"%04X",nLen);
		memset(m_strVa,0x00,512);
		sprintf(m_strVa,"%s%s%s","3082",stroid,m_snmpBuf);
printf("buf:%s\n",m_strVa);
		memset(m_snmpBuf,0x00,500);
		nLen = HexToDec(m_snmpBuf,m_strVa,strlen(m_strVa));

		if ( pSnmpPacket )
		{
			pSnmpPacket->sPackLen = nLen;
			memcpy(pSnmpPacket->Buf,m_snmpBuf,nLen);

		}
		

/*
		int nSize = HexToDec(m_snmpBuf,strSnmpPack);//
		if ( pSnmpPacket )
		{
			pSnmpPacket->sPackLen = nSize;
			memcpy(pSnmpPacket->Buf,m_snmpBuf,nSize);

		}*/



	return 0;
}

int main()
{
	int snmpfd;
	int snmpdstfd;
	struct sockaddr_in servaddr;

	struct sockaddr from;
	int fromlen;

	struct sockaddr profrom;
	int profromlen;

	int retStatus;

	int s,len;
	struct sockaddr_in addr;
	int addr_len =sizeof(struct sockaddr_in);
	char buffer[1024];

	SNMPPACKET pSnmpPack;
	long dwID = 1;
	char * lpszOid[17];
	int i;
	strcpy(lpszOid,"1.3.6.1.4.1.16985.1.12.1.1.1");


	if((s = socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket");
		exit(1);
	}
	if((snmpfd = socket(AF_INET,SOCK_DGRAM,0))<0){
		perror("socket");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(161);

	retStatus = bind(snmpfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	if (retStatus < 0) 
	{
		printf("Unable to bind to socket (%d).\n", errno);
		exit(-1);
	}
	printf("Started the SNMP agent\n");


	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	for ( ; ; ) 
	{

		fromlen = sizeof(from);
		len = recvfrom(snmpfd, buffer, 1024, 0, &from, &fromlen);

		printf("Received datagram from %s, port %d\n", 
           inet_ntoa(((struct sockaddr_in *)&from)->sin_addr),
           ((struct sockaddr_in *)&from)->sin_port);

		if (len >= 0) 
		{

#if 1

        printf("\nRequest: \n");
        for (i = 0 ; i < len ; i++) 
		{
          if ((i % 16) == 0) printf("\n  %04x : ", i);
          printf("%02x ", (unsigned char)buffer[i]);
        }
        printf("\n\n");

#endif

		//接收到解开buffer,然后更改数据，再按客户端发消息一样发到目的，然后接收后直接转发
		snmptest(buffer,len);

	 //   CreateGetPacket(lpszOid,strlen(lpszOid),1,/*"GLBLTECH",*/dwID,&pSnmpPack);
/*
		if(pSnmpPack.Buf)
		{
			printf("send.........\n");
			len =sendto(s,(const char *)(pSnmpPack.Buf),pSnmpPack.sPackLen,0,(struct sockaddr *)&addr,addr_len); 
			if(len <0)
			{
				printf("sendto err!\n");
			}
			bzero(buffer,sizeof(buffer));
			len = recvfrom(s, buffer, sizeof(buffer), 0, &profrom, &profromlen);
			if (len >= 0) 
			{//

			
				sendto(snmpfd,(const char *)buffer,len,0,(struct sockaddr *)&from,fromlen); 
#if 1

	
				printf("\nRequest: \n");
				for (i = 0 ; i < len ; i++) 
				{
				  if ((i % 16) == 0) printf("\n  %04x : ", i);
				  printf("%02x ", (unsigned char)buffer[i]);
				}
				printf("\n\n");

#endif
			}//


		}
		*/
		
		}
	}
	  close(s);
	  close(snmpfd);
	  return 0;

  }



