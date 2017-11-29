// FileSystem.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <cstdlib>
#include "FileManeger.h"



int main()
{
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
		if (opt != "pwd"&&opt!="pwd_r")
			cin >> content;
		if (opt == "echo" || opt == "cpy"||opt=="cpydir")
			cin >> str;
		if (opt == "echo")
			swap(content, str);//理解错题意了，这样改比较快
		cout<<fm->exec(opt, content,str);
		if (opt == "echo" || opt == "mkdir" || opt == "rm" || opt == "rmdir" || opt == "cpy" || opt == "cpydir")
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
    return 0;
}

