// FileSystem.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <cstdlib>
#include "FileManeger.h"



int main()
{
	//FILE *f = fopen("text.out", "w");
	/*int a;
	fscanf(f, "%d",&a);
	fseek(f, 10, ftell(f));
	char c;
	while (c=fgetc(f))
		cout <<c<<endl;*/

	//fseek(f, 10240000, SEEK_CUR);
	//fprintf(f, "%d", 1);

	FileManeger *fm = new FileManeger("data");
	string opt;
	while (cout << ">>", cin >> opt)
	{
		string content;
		string str;
		if (opt == "exit")
		{
			delete fm;
			break;
		}
		if (opt != "pwd")
			cin >> content;
		if (opt == "echo")
			cin >> str;
		cout<<fm->exec(opt, content,str);
		if (opt == "echo" || opt == "mkdir" || opt == "rm" || opt == "rmdir")
		{
			cout << "write the change? y/n\n>>";
			string ans;
			cin >> ans;
			if (ans == "y")
			{
				fm->exec("write", content, str);
			}
		}
	}
	//while (true);
    return 0;
}

