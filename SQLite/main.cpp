#include <stdio.h>
   
#include "SQLite.h"
#pragma comment(lib,"./SQLite3/sqlite3.lib") 

int main()
{
	SQLite s;
	s.Open("./Test.db");
	SQLiteDataReader r = s.ExcuteQuery("select * from test");
	while(r.Read())
	{
		for(int i = 0; i < r.ColumnCount(); ++i)
		{
			printf("%s:%s\n", r.GetName(i), r.GetStringValue(i).c_str());
		}
	}
	return 0;
}