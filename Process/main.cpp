#include "Process.h"
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

void main()
{
	Process p;
	p.Create("cmd");

	p.Write("dir\n");

	string data;
	while(1)
	{
		data = "";
		p.Read(data);
		printf(data.c_str());
    }
}