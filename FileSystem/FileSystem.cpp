// FileSystem.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "FileManeger.h"
#include <iostream>
using namespace std;
int main()
{
	//freopen("longfile.txt", "r", stdin);
	//freopen("output.txt", "w", stdout);
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
		if (opt != "pwd" && opt != "pwd_r")
			cin >> content;
		if (opt == "echo" || opt == "cpy" || opt == "cpydir" || opt == "append")
			cin >> str;
		if (opt == "echo" || opt == "append")
			swap(content, str);//���������ˣ������ıȽϿ�
		cout<<fm->exec(opt, content,str);
		if (opt == "echo" || opt == "mkdir" || opt == "rm" || opt == "rmdir" || opt == "cpy" || opt == "cpydir")
			fm->exec("write", content, str);
	}
    return 0;
}

