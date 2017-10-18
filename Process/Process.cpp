#include "Process.h"

Process::Process()
{
	m_hStdInR = NULL;
	m_hStdInW = NULL;
	m_hStdOutR = NULL;
	m_hStdOutW = NULL;
	m_hStdErrW = NULL;
	memset(&m_ProcInfo, 0, sizeof(m_ProcInfo));
}

Process::~Process()
{
	Release();
}

BOOL Process::Create(LPSTR szCmd)
{
	Release();
	SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

	//产生一个用于stdin的管道，得到两个HANDLE:  hStdInRead用于子进程读出数据，hStdInWrite用于主程序写入数据  
	if(!CreatePipe(&m_hStdInR, &m_hStdInW, &sa, 0)) return FALSE;
	//产生一个用于stdout的管道，得到两个HANDLE:  hStdInRead用于主程序读出数据，hStdInWrite用于子程序写入数据
	if(!CreatePipe(&m_hStdOutR, &m_hStdOutW, &sa, 0)) return FALSE;
	//由于stderr一般就是stdout, 直接复制句柄hStdOutWrite，得到 hStdErrWrite
	if (!DuplicateHandle(GetCurrentProcess(), m_hStdOutW, GetCurrentProcess(), &m_hStdErrW, 0, TRUE, DUPLICATE_SAME_ACCESS)) return FALSE;

	STARTUPINFO siStartInfo = {0};
	siStartInfo.cb = sizeof(STARTUPINFO);
#if 1	/*注释下面三行就是普通的创建子进程*/
	siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;  
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;  
	siStartInfo.wShowWindow = SW_HIDE;
#endif
	siStartInfo.hStdOutput = m_hStdOutW;
	siStartInfo.hStdError =  m_hStdErrW;
	siStartInfo.hStdInput = m_hStdInR;

	if(!CreateProcess(
			NULL,  
			szCmd,				    // 子进程的命令行  
			NULL,                   // process security attributes  
			NULL,                   // primary thread security attributes  
			TRUE,                   // handles are inherited  
			0,                      // creation flags  
			NULL,                   // use parent's environment  
			NULL,                   // use parent's current directory  
			&siStartInfo,	        // STARTUPINFO pointer  
			&m_ProcInfo))           // receives PROCESS_INFORMATION
		return FALSE;

	return TRUE;
}

void Process::Release()
{
	if(m_hStdInR) { CloseHandle(m_hStdInR); m_hStdInR = NULL; }
	if(m_hStdInW) { CloseHandle(m_hStdInW); m_hStdInW = NULL; }
	if(m_hStdOutR) { CloseHandle(m_hStdOutR); m_hStdOutR = NULL; }
	if(m_hStdOutW) { CloseHandle(m_hStdOutW); m_hStdOutW = NULL; }
	if(m_hStdErrW) { CloseHandle(m_hStdErrW); m_hStdErrW = NULL; }
	if(IsLive()) TerminateProcess(m_ProcInfo.hProcess, 0);
	memset(&m_ProcInfo, 0, sizeof(m_ProcInfo));
}

BOOL Process::Write(LPCSTR szMsg)
{
	DWORD dwWritten;
	return WriteFile(m_hStdInW, szMsg, strlen(szMsg), &dwWritten, NULL);
}

BOOL Process::Peek()
{
	DWORD bytes = 0;
	PeekNamedPipe(m_hStdOutR, NULL, NULL, NULL, &bytes, NULL);
	return bytes > 0;
}

std::string& Process::Read(std::string &data)
{
	char out_buffer[4096] = {0};  
	DWORD dwRead = 0;

	while(Peek())
	{
		if(ReadFile(m_hStdOutR, out_buffer, sizeof(out_buffer), &dwRead, NULL) && dwRead > 0)
			data.append(out_buffer, dwRead);
		else break;
	}
	
	return data;
}

BOOL Process::IsLive(DWORD dwWait)
{
	if(dwWait != 0 && WaitForSingleObject(m_ProcInfo.hProcess, dwWait) == WAIT_OBJECT_0) return FALSE;

	DWORD exitCode = 0;
	GetExitCodeProcess(m_ProcInfo.hProcess, &exitCode);
	return exitCode == STILL_ACTIVE;
}