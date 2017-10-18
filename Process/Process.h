/*
	创建子进程，并重定向子进程的stdout、stdin到父进程的管道
	让父进程可以向用户敲命令行一样操作子进程
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
	//数据命令行，创建支持输入输出重定向的子进程
	BOOL Create(LPSTR szCmd);
	//释放进程数据
	void Release();
	//向子进程输入数据
	BOOL Write(LPCSTR szMsg);
	//查看子进程是否有数据输出
	BOOL Peek();
	//读出子进程所有输出数据
	std::string& Read(std::string &data);
	//判断子进程是否存活,nWait:等待时间(毫秒)
	BOOL IsLive(DWORD dwWait = 0);
private:
	//定义句柄: 构成stdin管道的两端句柄  
	HANDLE  m_hStdInR;         //子进程用的stdin的读入端  
	HANDLE  m_hStdInW;        //主程序用的stdin的读入端  
	//定义句柄: 构成stdout管道的两端句柄  
	HANDLE  m_hStdOutR;     ///主程序用的stdout的读入端  
	HANDLE  m_hStdOutW;    ///子进程用的stdout的写入端  
	//定义句柄: 构成stderr管道的句柄，由于stderr一般等于stdout,我们没有定义hStdErrRead，直接用hStdOutRead即可  
	HANDLE  m_hStdErrW;       ///子进程用的stderr的写入端

	PROCESS_INFORMATION m_ProcInfo;
};


#endif