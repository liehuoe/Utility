#if !defined(_MY_DATABASE_H_)
#define _MY_DATABASE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include "include/mysql.h"

class CMyResult
{
friend class CMyDB;
private:
	CMyResult() : m_res(NULL) {}
	~CMyResult() { Release(); }

	void Set(MYSQL_RES *res)
	{
		if(!res) return;
		m_res = res;
	}
	void Release() { if(m_res) mysql_free_result(m_res); }
public:
	BOOL Next() { return mysql_fetch_row(m_res) != NULL; }
	const char* GetString(unsigned int index)
	{
		if(m_res->eof || index >= m_res->field_count) return NULL;
		return m_res->current_row[index];
	}
	long GetLong(unsigned int index)
	{
		return atol(GetString(index));
	}
private:
	MYSQL_RES *m_res;
};

class CMyDB
{
public:
	CMyDB()
	{
		mysql_init(&m_mysql);
		//设置自动重连
		my_bool bVal = 1;
		mysql_options(&m_mysql,MYSQL_OPT_RECONNECT, &bVal);
	}
	~CMyDB()
	{
		mysql_close(&m_mysql);
	}

	//其他线程使用前主线程调用,关于多线程安全的问题请查官方文档
	static void Init() { mysql_library_init(0, NULL, NULL); }
	//连接到数据库
	BOOL Connect(
		const char *host,
		const char *user,
		const char *passwd,
		const char *db,
		unsigned int port)
	{
		if(!mysql_real_connect(&m_mysql, host, user, passwd, db, port, NULL, 0))
		{
			return FALSE;
		}
		mysql_set_character_set(&m_mysql,"gb2312");
		
		return TRUE;
	}
	//执行SELECT语句,SELECT结果通过CMyResult返回,本次SELECT会使上次SELECT的结果CMyResult无效
	CMyResult* Query(const char *sql)
	{
		m_result.Release();
		if(mysql_query(&m_mysql, sql) == 0)
		{
			m_result.Set(mysql_use_result(&m_mysql));
		}
		return &m_result;
	}
	//执行非SELECT语句
	BOOL Execute(const char *sql)
	{
		return (mysql_query(&m_mysql, sql) == 0);
	}

	//返回上一次错误的描述信息,如果没有错误则返回NULL
	const char* GetError() { return mysql_error(&m_mysql); }
private:
	MYSQL m_mysql;
	CMyResult m_result;
};

#endif