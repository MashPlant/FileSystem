#include "stdafx.h"
#include "FileManager.h"
#include <iostream>
using namespace  std;
void init()
{
	freopen("testfull.txt", "w", stdout);
	for (int i=0;i<14;++i)
	{
		cout << "mkdir /" + to_string(i) + "\n";
		for (int j=0;j<14;++j)
		{
			cout << "mkdir /" + to_string(i) +"/"+ to_string(j)+ "\n";
			//for (int k=0;k<=14;++k)
			//{
			//	cout << "mkdir /" + to_string(i) + "/" + to_string(j) + "/" + to_string(k) + "\n";
			//}
		}
	}
}
int main()
{
	//init();
	//freopen("testfull.txt", "r", stdin);
	//freopen("out.txt", "w", stdout);
	clock_t beg = clock();
	FileManager *fm = new FileManager("data");
	cout << clock() - beg << endl;
	std::string status;
	vector<string> tmp;
	while (std::cout << ">>", tmp=readLine(),std::cout << (status = fm->exec(tmp)), status != "exited");
	delete fm;
    return 0;
}

