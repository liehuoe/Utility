/*
	�����ӽ��̣����ض����ӽ��̵�stdout��stdin�������̵Ĺܵ�
	�ø����̿������û���������һ�������ӽ���
*/
#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <windows.h>
#include <string>

class Process
{
public:
	Process();
	~Process();
	//���������У�����֧����������ض�����ӽ���
	BOOL Create(LPSTR szCmd);
	//�ͷŽ�������
	void Release();
	//���ӽ�����������
	BOOL Write(LPCSTR szMsg);
	//�鿴�ӽ����Ƿ����������
	BOOL Peek();
	//�����ӽ��������������
	std::string& Read(std::string &data);
	//�ж��ӽ����Ƿ���,nWait:�ȴ�ʱ��(����)
	BOOL IsLive(DWORD dwWait = 0);
private:
	//������: ����stdin�ܵ������˾��  
	HANDLE  m_hStdInR;         //�ӽ����õ�stdin�Ķ����  
	HANDLE  m_hStdInW;        //�������õ�stdin�Ķ����  
	//������: ����stdout�ܵ������˾��  
	HANDLE  m_hStdOutR;     ///�������õ�stdout�Ķ����  
	HANDLE  m_hStdOutW;    ///�ӽ����õ�stdout��д���  
	//������: ����stderr�ܵ��ľ��������stderrһ�����stdout,����û�ж���hStdErrRead��ֱ����hStdOutRead����  
	HANDLE  m_hStdErrW;       ///�ӽ����õ�stderr��д���

	PROCESS_INFORMATION m_ProcInfo;
};


#endif