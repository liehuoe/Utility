#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <winsvc.h>
#include <stdio.h>
#include <string.h>
#include <UserEnv.h>
#include <wtsapi32.h>
#include <time.h>
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Userenv.lib")

#define SERVICE_TYPE		(SERVICE_WIN32_SHARE_PROCESS|SERVICE_INTERACTIVE_PROCESS)
#define SERVICE_NAME "daemon"
#define SERVICE_DISPLAY_NAME "daemon"
#define LOG_FILE "error.log"

SERVICE_STATUS_HANDLE hStatus;
DWORD dwCurrentStatus = SERVICE_STOPPED;

HANDLE hToken = NULL;
LPVOID environment;

int nProcessCount;
char *szProcess[256];

BOOL Init();
void Release();
HANDLE Exec(char *cmd);
void Log(char* str, ...);

void ServiceMain(int argc, char** argv);
void ServiceStart();
void ReportStatus(DWORD state);
void ControlHandler(DWORD request);

int OnInstall();
int OnRemove();
int OnStart();
int OnStop();
int OnStatus();

BOOL Init()
{
	/////////////////设置工作目录/////////////////////
	char dir[MAX_PATH];
    GetModuleFileName(NULL, dir, MAX_PATH);
	*strrchr(dir, '\\') = '\0';
	SetCurrentDirectory(dir);
	/////////////////初始化UI环境/////////////////////
	if(!WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &hToken))
	{
		Log("%s WTSQueryUserToken is failed", __FUNCTION__);
		return FALSE;
	}
	if(!ImpersonateLoggedOnUser(hToken))
	{
		Log("%s ImpersonateLoggedOnUser is failed", __FUNCTION__);
		return FALSE;
	}
	//获取到当前用户路径
	TCHAR szUsernamePath[MAX_PATH];
	DWORD UsernamePathSize = ARRAYSIZE(szUsernamePath);
	if (!GetUserProfileDirectory(hToken, szUsernamePath, &UsernamePathSize))
	{
		Log("%s GetUserProfileDirectory is failed", __FUNCTION__);
		return FALSE;
	}
	//创建环境
	if (!CreateEnvironmentBlock(&environment, hToken, FALSE))
	{
		Log("could not create environment block (error: %i)", GetLastError());
		return FALSE;
	}
	/////////////////配置文件/////////////////////
	FILE *fp;
	errno_t err = fopen_s(&fp, "config", "r");
	if(err != 0)
	{
		Log("failed to open config: %d", err);
		return FALSE;
	}
	fseek(fp,0L,SEEK_END);
	int size = ftell(fp);
	if(size <= 0) return FALSE;
	char *p = szProcess[nProcessCount++] = (char*)malloc(size + 1);
	rewind(fp);
	//找有效开头
	int ch;
	while((ch = fgetc(fp)) != EOF)
	{
		if(ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t') continue;
		*p++ = ch;
		break;
	}
	BOOL bFind = FALSE;
	while((ch = fgetc(fp)) != EOF)
	{
		if(bFind)
		{
			if(ch == '\r' || ch == '\n' || ch == ' ' || ch == '\t')
			{
				*p++ = '\0';
			}
			else
			{
				szProcess[nProcessCount++] = p;
				*p++ = ch;
				bFind = FALSE;
			}
		}
		else
		{
			if(ch == '\r' || ch == '\n')
			{
				*p++ = '\0';
				bFind = TRUE;
			}
			else
			{
				*p++ = ch;
			}
		}
	}
	*p = '\0';
	fclose(fp);
	if(nProcessCount <= 0)
	{
		Log("找不到有效配置");
		return FALSE;
	}

	return TRUE;
}

void Release()
{
	ReportStatus(SERVICE_STOP_PENDING);
	DestroyEnvironmentBlock(environment);
	CloseHandle(hToken);
}

int main(int argc, char* argv[])
{
    if(argc == 1)
	{
		SERVICE_TABLE_ENTRY ServiceTable[2] = {{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain }};
		StartServiceCtrlDispatcher(ServiceTable);
		return 0;
	}
	if(!_stricmp(argv[1],"install")) return OnInstall();
	else if(!_stricmp(argv[1],"remove")) return OnRemove();
	else if(!_stricmp(argv[1],"start")) return OnStart();
	else if(!_stricmp(argv[1],"stop")) return OnStop(); 
	else if(!_stricmp(argv[1],"status")) return OnStatus(); 
	else
	{
		printf("invailid parameter\n");
		return -1;
	}
}

void Log(char* str, ...)
{
    FILE* fp = NULL;
	fopen_s(&fp, LOG_FILE, "a+");
    if(fp == NULL)
    {
        printf("failed to open file: %d\n", GetLastError());
        return;
    }
	time_t t = time(0);
	tm lt;
	localtime_s(&lt, &t);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S|", &lt);
	fprintf(fp, tmp);
	va_list ap;
	va_start(ap, str);
	vfprintf(fp, str, ap);
	va_end(ap);
	fprintf(fp, "\r\n");
    fclose(fp);
}

void ReportStatus(DWORD state)
{
	dwCurrentStatus = state;
	SERVICE_STATUS status = {
		SERVICE_TYPE,										//服务类型
		state,												//当前状态
		SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN		//可接受的操作
	};
	SetServiceStatus(hStatus, &status);
}

void ServiceMain(int argc, char** argv)
{
	hStatus = RegisterServiceCtrlHandler(SERVICE_NAME, (LPHANDLER_FUNCTION)ControlHandler);
    if(!hStatus) return;
	ReportStatus(SERVICE_START_PENDING);
	if(!Init())
	{
		ReportStatus(SERVICE_STOPPED);
		return;
	}
	ReportStatus(SERVICE_RUNNING);
	ServiceStart();
	ReportStatus(SERVICE_STOPPED);
}

void ServiceStart()
{
	byte setCount = 0;//等待数量
	byte *setIndex = (byte *)malloc(nProcessCount * sizeof(byte *));//等待的进程下标
	HANDLE *hSet = (HANDLE *)malloc(nProcessCount * sizeof(HANDLE *));//等待的句柄数组
	byte failCount = 0;//出错数量
	byte *failIndex = (byte *)malloc(nProcessCount * sizeof(byte *));//出错的进程下标
	for(int i = 0; i < nProcessCount; ++i) failIndex[failCount++] = i;
	while(1)
	{
		//启动出错进程
		byte count = failCount;
		failCount = 0;
		for(int i = 0; i < count; ++i)
		{
			HANDLE handle = Exec(szProcess[failIndex[i]]);
			if(handle == NULL)
			{
				if(failCount == i) ++failCount;
				else failIndex[failCount++] = failIndex[i];
			}
			else
			{
				hSet[setCount] = handle;
				setIndex[setCount++] = failIndex[i];
			}
		}
		//监控进程退出
		DWORD ret = WaitForMultipleObjects(setCount, hSet, FALSE, 1000);
		if(ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + setCount)
		{
			ret -= WAIT_OBJECT_0;
			Log("进程(%s)退出", szProcess[setIndex[ret]]);
			//整理数据
			CloseHandle(hSet[ret]);
			failIndex[failCount++] = setIndex[ret];
			for(int i = ret + 1; i < setCount; ++i)
			{
				hSet[i - 1] = hSet[i];
				setIndex[i - 1] = setIndex[i];
			}
			--setCount;
		}
		else if(ret != WAIT_TIMEOUT)
		{
			Log("WaitForMultipleObjects 出错: %d", GetLastError());
		}
		if(dwCurrentStatus != SERVICE_RUNNING)
		{
			//清理进程
			for(int i = 0; i < setCount; ++i)
			{
				TerminateProcess(hSet[i], 0);
			}
			break;
		}
		Sleep(1000);
	}
}

void ControlHandler(DWORD request)
{
    switch(request)
    {
        case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			Release();
            return;
        default:
            break;
    }
	ReportStatus(dwCurrentStatus);
}

int OnInstall()
{
	int code = -1;
    SC_HANDLE scm = OpenSCManager(0, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm == NULL)
    {
		code = GetLastError();
        printf("OpenSCManager error:%d\n", code);
        return code;
    }
	char filename[MAX_PATH];
    GetModuleFileName(NULL, filename, MAX_PATH);
    printf("Installing Service... ");
    SC_HANDLE scv = CreateService(scm,//句柄
		SERVICE_NAME,//服务开始名
		SERVICE_DISPLAY_NAME,//显示服务名
		SERVICE_ALL_ACCESS, //服务访问类型
		SERVICE_TYPE,//服务类型
		SERVICE_AUTO_START, //自动启动服务
		SERVICE_ERROR_IGNORE,//忽略错误
		filename,//启动的文件名
		NULL,//name of load ordering group (载入组名)
		NULL,//标签标识符
		NULL,//相关性数组名
		NULL,//帐户(当前)
		NULL); //密码(当前)
    if(scv == NULL)
    {
        code = GetLastError();
        if(code != ERROR_SERVICE_EXISTS)
        {
            printf("Failed");
        }
        else
        {
            printf("Already Exists");
			code = 0;
        }
    }
    else
    {
        printf("Success");
        CloseServiceHandle(scv);
		code = 0;
    }
    CloseServiceHandle(scm);
	return code;
}

int OnRemove()
{
	int code = -1;
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm == NULL)
    {
		code = GetLastError();
        printf("OpenSCManager error:%d\n", code);
        return code;
    }
    SC_HANDLE scv = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (scv == NULL)
    {
		code = GetLastError();
        printf("OpenService error:%d\n", code);
		CloseServiceHandle(scm);
        return code;
    }
	SERVICE_STATUS status = {0};
    QueryServiceStatus(scv, &status);
    if (status.dwCurrentState == SERVICE_RUNNING)
    {
        ControlService(scv, SERVICE_CONTROL_STOP, &status);
    }
	printf("Removing Service... ");
    if(DeleteService(scv))
	{
		printf("Success");
		code = 0;
	}
	else printf("Failed");
    CloseServiceHandle(scv);
	CloseServiceHandle(scm);
	return code;
}


int OnStart()
{
    int code = -1;
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm == NULL)
    {
		code = GetLastError();
        printf("OpenSCManager error:%d\n", code);
        return code;
    }
    SC_HANDLE scv = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (scv == NULL)
    {
		code = GetLastError();
        printf("OpenService error:%d\n", code);
		CloseServiceHandle(scm);
        return code;
    }
	printf("Starting Service .... ");
	if(StartService(scv, 0, NULL))
	{
		printf("Success");
		code = 0;
	}
	else
	{
		code = GetLastError();
		if(code == ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("Already Running !\n");
			code = 0;
		}
	}
	CloseServiceHandle(scv);
	CloseServiceHandle(scm);
	return code;
}


int OnStop()
{
	int code = -1;
    SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scm == NULL)
    {
		code = GetLastError();
        printf("OpenSCManager error:%d\n", code);
        return code;
    }
    SC_HANDLE scv = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (scv == NULL)
    {
		code = GetLastError();
        printf("OpenService error:%d\n", code);
		CloseServiceHandle(scm);
        return code;
    }
	SERVICE_STATUS status = {0};
	QueryServiceStatus(scv, &status);
	if(status.dwCurrentState == SERVICE_RUNNING)
	{
		printf("Stoping Service... ");
		if(ControlService(scv, SERVICE_CONTROL_STOP, &status))
		{
			printf("Success");
			code = 0;
		}
		else printf("Failed");
	}
	else
	{
		printf("Service is not running!");
		code = 0;
	}
    CloseServiceHandle(scv);
	CloseServiceHandle(scm);
	return code;
}

int OnStatus()
{
	int code = -1;
	SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (scm == NULL)
	{
		code = GetLastError();
		printf("OpenSCManager error:%d\n", code);
		return code;
	}
	SC_HANDLE scv = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
	if (scv == NULL)
	{
		code = GetLastError();
		printf("OpenService error:%d\n", code);
		CloseServiceHandle(scm);
		return code;
	}
	SERVICE_STATUS status = {0};
	QueryServiceStatus(scv, &status);
	printf("Service Status: %s", status.dwCurrentState == SERVICE_RUNNING ? "Running" : "Stopped");
	CloseServiceHandle(scv);
	CloseServiceHandle(scm);
	return 0;
}

HANDLE Exec(char *cmd)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO         si = {0};
	si.cb        = sizeof(STARTUPINFO);
	si.lpDesktop = "winsta0\\default";
	if(!CreateProcessAsUser(
		hToken,
		NULL,
		cmd,
		NULL,
		NULL,
		FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT,
		environment,
		NULL,
		&si,
		&pi
	) || pi.hProcess == INVALID_HANDLE_VALUE)
	{
		Log("(%s)启动失败: %d", cmd, GetLastError());
		return NULL;
	}
	CloseHandle(pi.hThread);
	return pi.hProcess;
}