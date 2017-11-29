// FileSystem.cpp : �������̨Ӧ�ó������ڵ㡣
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
			swap(content, str);//���������ˣ������ıȽϿ�
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

