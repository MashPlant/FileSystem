#include "stdafx.h"
#include "FileManager.h"
#include <memory>
#include <iostream>

int main()
{
	std::shared_ptr<FileManager> fm = std::make_shared<FileManager>("data");
	std::string status;
	while (std::cout << ">>",std::cout << (status = fm->exec(readLine())), status != "exited");
	return 0;
}