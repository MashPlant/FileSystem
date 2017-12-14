#include "stdafx.h"
#include "FileManeger.h"
#include <iostream>
using namespace std;
int main()
{
	FileManeger *fm = new FileManeger("data");
	string status;
	while (cout << ">>", cout << (status = fm->exec(readLine())), status != "exited");
	delete fm;
    return 0;
}

