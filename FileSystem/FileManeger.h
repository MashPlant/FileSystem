#ifndef FILEMANEGER_H
#define FILEMANEGER_H
#include <string>
#include <cstdio>
#include <map>
#include <set>
#include <regex>
#include "Util.h"

class FileManeger
{
private:
	const static int MAX_SIZE = 4096;
	const static int PATH_SIZE = 252;
	const static int FIL = 0;
	const static int DIR = 1;
	const static int EMPTY = 2;

	const static std::string NON_EXIST;
	const static std::string NON_FILE;
	const static std::string NON_DIR;
	const static std::string LEN_ERROR;
	const static std::string IS_EMPTY;
	const static std::string ALREADY_EXIST;
	const static std::string TOO_LONG;
	const static std::string FULL_DIR;
	const static std::string FULL_SYS;

	const static std::set<std::string> REQUIRE_FILE;
	const static std::set<std::string> REQUIRE_DIR;
	const static std::set<std::string> REQUIRE_EXIST;
	const static std::set<std::string> REQUIRE_PARENT;
	const static std::set<std::string> ALL_COMMAND;
	
	bool inodeBitmap[MAX_SIZE];
	bool blockBitmap[MAX_SIZE];
	std::map<int, int> blockToInode;
	int curr = 0;
	typedef std::pair<int, int> pii;

	struct Inode
	{
		int id = 0;
		int mode = EMPTY;
		int sz = 0;
		int block = 0;
		int cnt = 0;//占据连续的一片block
		int p = 0 ;
		int useless = 0;
		void read(FILE *f)
		{
			fread(f, id, mode, sz, block, cnt, p);
			for (int i = 0; i < 2; ++i)
				fread(f,useless);
		}
		void print(FILE *f)
		{
			fwrite(f, id, mode, sz, block, cnt, p);
			for (int i = 0; i < 2; ++i)
				fwrite(f,useless);
		}
	};
	struct FileBlock
	{
		char data[MAX_SIZE] = { '\0' };
	};

	struct DirEntry
	{
		char name[PATH_SIZE] = {'\0'};
		int inode = 0;
	};

	struct DirBlock
	{
		DirEntry dirs[16];
		//14表示当前目录，15表示上级目录，不允许使用
	};

	class Allocator
	{
	private:
		FileManeger *self = nullptr;
		//约定所有的parent都是指parent的inode
		int allocateInode(int mode, int p);
		int allocateBlock(int parent, int nInode, int mode, int size = 1);
		void allocateEntry(int parent, int nInode, const std::string &content);
		void initDirBlock(int nInode, int nBlock, int parent, const std::string &content, const std::string &parent_name);
	public:
		//返回值:新申请到的inode
		int createFile(int parent, const std::string &name, std::string source);
		int createDir(int parent, const std::string &name, const std::string &parentName);
		Allocator(FileManeger *_self = nullptr) :self(_self) {}
	};
	friend Allocator;
	Allocator allocator;

	FileBlock fbs[MAX_SIZE];
	DirBlock dbs[MAX_SIZE];

	Inode inodes[MAX_SIZE];
	std::string path;

	int getP(int inode);
	int getMode(int inode);
	std::string getData(int inode);
	std::string getName(int inode);

	//覆盖所有可能情形,不需要对应情形时传适当的参数即可.
	std::string check(const std::string &name, int parent, int requiredInodes, int requiredBlocks);
	pii nameToInode(const std::string &name);//first:self,second:parent
	void cpy(int dest, int sour);
	void remove(int inode);
	std::string print(int inode, int shift, bool r);
	std::string find(int inode, const std::string &match, std::regex *re);
	void init();
	void write(FILE *f);
	bool getAbsPath(std::string &content);
	void resize(int inode, int delta);
	pii count(int inode);//first:inode, second block
public:
	FileManeger(const std::string &_path);
	~FileManeger();
	std::string exec(std::string opt, std::string content, std::string str);
};

#endif

