#pragma once
#include <string>
#include <iostream>
#include <cstdio>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
#include <regex>
#include "Util.h"
using namespace std;

class FileManeger
{
private:
	const static int MAX_SIZE = 4096;
	const static int PATH_SIZE = 252;
	const static int FIL = 0;
	const static int DIR = 1;
	const static int EMPTY = 2;

	const static string NON_EXIST;
	const static string NON_FILE;
	const static string NON_DIR;
	const static string LEN_ERROR;
	const static string IS_EMPTY;
	const static string ALREADY_EXIST;
	const static string TOO_LONG;
	const static string FULL_DIR;
	const static string FULL_SYS;

	const static set<string> REQUIRE_FILE;
	const static set<string> REQUIRE_DIR;
	const static set<string> REQUIRE_EXIST;
	const static set<string> REQUIRE_PARENT;
	const static set<string> ALL_COMMAND;
	
	bool inodeBitmap[MAX_SIZE];
	bool blockBitmap[MAX_SIZE];
	int curr = 0;

	struct Inode
	{
		int id = 0;
		int mode = EMPTY;
		int sz = 0;
		int block = 0;
		int nextInode = -1;//数组模拟链表，从而实现存放多个blocks
		int useless = 0;
		void read(FILE *f)
		{
			fread(f, id, mode, sz, block);
			for (int i = 0; i < 4; ++i)
				fread(f,useless);
		}
		void print(FILE *f)
		{
			fwrite(f, id, mode, sz, block);
			for (int i = 0; i < 4; ++i)
				fwrite(f,useless);
		}
	};
	struct FileBlock
	{
		char data[MAX_SIZE] = { '\0' };
		int p;//指向父亲inode
	};

	struct DirEntry
	{
		char name[PATH_SIZE] = {'\0'};
		int inode = 0;
	};

	//"自己不知道自己的名字"

	struct DirBlock
	{
		DirEntry dirs[16];
		int p;
		//14表示当前目录，15表示上级目录，不允许使用
		//这里只包含名字和索引信息，相当于只是虚拟的文件/文件夹
		//文件/文件夹的实体需要用inode在Inode[]里找到对应索引，再通过索引找到block
	};
	FileBlock fbs[MAX_SIZE];
	DirBlock dbs[MAX_SIZE];

	Inode inodes[MAX_SIZE];
	string path;

	int getP(int inode)
	{
		if (inodes[inode].mode == FIL)
			return fbs[inodes[inode].block].p;
		if (inodes[inode].mode == DIR)
			return dbs[inodes[inode].block].p;
	}

	int getKind(int inode)
	{
		return inodes[inode].mode;
	}
	
	//覆盖所有可能情形,不需要对应情形时传适当的参数即可.
	string check(const string &name,int parent,int requiredInodes,int requiredBlocks)
	{
		if (name.size() >= PATH_SIZE)
			return TOO_LONG;
		if (parent!=-1)//不要求检查parent
		{
			bool freeDir = false;
			DirBlock &db = dbs[inodes[parent].block];
			for (int i = 0; i<14; ++i)
			{
				if (!db.dirs[i].name[0])
					freeDir = true;
			}
			if (!freeDir)
				return FULL_DIR;
		}
		for (int i = 0; i<MAX_SIZE; ++i)
		{
			if (!inodeBitmap[i])
				--requiredInodes;
			if (!blockBitmap[i])
				--requiredBlocks;
		}
		if (requiredBlocks > 0 || requiredInodes > 0)
			return FULL_SYS;
		return "";
	}
	typedef pair<int, int> pii;
	pii nameToInode(const string &name)//first:self,second:parent
	{
		if (name == "" || name == "/")
		{
			return make_pair(0, -1);
		}
		vector<string> names = split(name, "/");
		int self = 0, parent = 0;
		if (names[0] == ".")//除非输入空串，否则分割一定有结果
		{
			self = curr;
			parent = getP(self);
		}
		if (names[0] == "..")
		{
			self = getP(curr);
			parent = getP(self);
		}
		if (names[0] != "."&&names[0] != ".."&&names[0] != "")
		{
			return make_pair(-1, -1);
		}
		for (int i = 1; i < names.size(); ++i)//names[0]只能是"." ".." ""之一,需跳过
		{
			bool found = false;
			for (int j = 0; j < 14; ++j)
			{
				if (getKind(self) == FIL)//如果真的要找文件，只可能在最后一项出现文件，此时i循环已退出
					return make_pair(-1, -1);
				string child = dbs[inodes[self].block].dirs[j].name;
				if (!child.empty() && child == names[i])
				{
					self = dbs[inodes[self].block].dirs[j].inode;
					parent = self;
					found = true;
					break;
				}
			}
			if (!found)
			{
				if (i + 1 != names.size())//仅仅在最后一项没找到是可以的，创建即可
					return make_pair(-1, -1);
				return make_pair(-1, parent);
			}
		}
		return make_pair(self, parent);
	}

	map<int, int> blockToInode;

	
	bool cpy(char *dest, const string &str,int maxSize)
	{
		if (str.size() >= maxSize)//由于'\0',不可相等
		{
			return false;
		}
		memcpy(dest, str.c_str(), str.size() * sizeof(char));
		dest[str.size()] = '\0';
		return true;
	}
	void cpy(int dest,int sour)//显然不能复制parent等属性
	{
		if (inodes[sour].mode == FIL)
		{
			memcpy(fbs[inodes[dest].block].data, fbs[inodes[sour].block].data, MAX_SIZE * sizeof(char));
		}
		else if (inodes[sour].mode == DIR)
		{
			DirBlock &ddb = dbs[inodes[dest].block];
			DirBlock &sdb = dbs[inodes[sour].block];
			for (int i = 0; i < 14; ++i)
			{
				string name = sdb.dirs[i].name;
				if (!name.empty())
				{
					string parent_name = ddb.dirs[14].name;
					if (inodes[sdb.dirs[i].inode].mode == DIR)
					{
						int nInode = allocateInode(DIR);
						int nBlock = allocateBlock(dest, nInode, DIR);
						initDirBlock(nInode, nBlock, dest, name, parent_name);
						allocateEntry(dest, nInode, name);
						cpy(nInode, sdb.dirs[i].inode);
					}
					else
					{
						int nInode = allocateInode(FIL);
						int nBlock = allocateBlock(dest, nInode, FIL);
						allocateEntry(dest, nInode, name);
						cpy(nInode, sdb.dirs[i].inode);
					}
				}
			}
		}
	}
	void remove(int inode)
	{
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			blockBitmap[inodes[inode].block] = false;
		}
		if (inodes[inode].mode == FIL)
		{
			FileBlock &fb = fbs[inodes[inode].block];
			DirBlock &p = dbs[inodes[fb.p].block];
			for (int i = 0; i < 14; ++i)
			{
				if (p.dirs[i].inode == inode)
				{
					memset(p.dirs[i].name, 0, sizeof(p.dirs[i].name));
					break;
				}
			}
			blockToInode.erase(inodes[inode].block);
		}
		else if (inodes[inode].mode == DIR)
		{
			DirBlock &db = dbs[inodes[inode].block];
			if (inode != 0)
			{
				DirBlock &p = dbs[inodes[db.p].block];
				for (int i = 0; i < 14; ++i)
				{
					if (p.dirs[i].inode == inode)
					{
						memset(p.dirs[i].name, 0, sizeof(p.dirs[i].name));
						break;
					}
				}
			}
			blockToInode.erase(inodes[inode].block);
			for (int i = 0; i < 14; ++i)
				if (db.dirs[i].name[0] != '\0')
					remove(db.dirs[i].inode);
		}
	}
	string print(int inode,int shift ,bool r)
	{
		string ret, space;
		for (int i = 0; i < shift; ++i)
			space += "   ";
		if (inodes[inode].mode == FIL)
			return "";
		Inode &in = inodes[inode];
		DirBlock &db = dbs[in.block];
		for (int i = 0; i < 14; ++i)
		{
			string child = db.dirs[i].name;
			if (!child.empty())
			{
				ret += space + (getKind(db.dirs[i].inode) == DIR ? "/" : "") + child + '\n';
				if (r)
					ret += print(db.dirs[i].inode,shift+1,r);
			}
		}
		return ret;
	}
	string find(int inode,const string &match,regex* re)
	{
		string ret;
		if (inodes[inode].mode == FIL)
			return "";
		Inode &in = inodes[inode];
		DirBlock &db = dbs[in.block];
		for (int i = 0; i < 14; ++i)
		{
			string child = db.dirs[i].name;
			if (!child.empty())
			{
				if (re ? regex_search(child, *re) : child.find(match) != string::npos)
					ret += (getKind(db.dirs[i].inode) == DIR ? "/" : "")+child + '\n';
				ret += find(db.dirs[i].inode, match, re);
			}
		}
		return ret;
	}

	void init()
	{
		FILE *f = fopen(path.c_str(), "wb");
		//superblock
		for (int j = 0; j < 2; ++j)
		{
			fwrite(f, static_cast<char>(0x80));
			writeEmpty(f, MAX_SIZE - 1);
		}
		//inode
		Inode empty, dir;
		empty.mode = EMPTY, dir.mode = DIR;
		dir.print(f);
		for (int i = 1; i < MAX_SIZE; ++i)
			empty.print(f);
		//block
		writeEmpty(f, MAX_SIZE*MAX_SIZE);
		fclose(f);
	}
	void write(FILE *f)
	{
		//superblock
		for (int i = 0; i < MAX_SIZE; ++i)
			fwrite(f, inodeBitmap[i]);
		for (int i = 0; i < MAX_SIZE; ++i)
			fwrite(f, blockBitmap[i]);
		//inode
		for (int i = 0; i < MAX_SIZE; ++i)
			inodes[i].print(f);
		//block
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			if (blockToInode.count(i))
			{
				if (inodes[blockToInode[i]].mode == DIR)
					for (int j = 0; j < 16; ++j)
					{
						fwrite(f, dbs[i].dirs[j].name);
						fwrite(f, dbs[i].dirs[j].inode);
					}
				else
					fwrite(f, fbs[i].data);
			}
			else
				writeEmpty(f, MAX_SIZE);

		}
	}

	int allocateInode(int mode)
	{
		int nInode = -1;
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			if (!inodeBitmap[i])
			{
				inodeBitmap[i] = true;
				inodes[i].id = i;
				inodes[i].mode = mode;
				inodes[i].sz = 0;
				nInode = i;
				break;
			}
		}
		return nInode;
	}
	int allocateBlock(int parent,int nInode,int mode)
	{
		int nBlock=-1;
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			if (!blockBitmap[i])
			{
				blockBitmap[i] = true;
				if (mode == DIR)
					dbs[i].p = parent;
				else if (mode == FIL)
					fbs[i].p = parent;
				inodes[nInode].block = i;
				blockToInode[i] = nInode;
				nBlock = i;
				break;
			}
		}
		return nBlock;
	}
	bool allocateEntry(int parent,int nInode,const string &content)
	{
		for (int i = 0; i < 14; ++i)
		{
			if (dbs[parent].dirs[i].name[0] == '\0')
			{
				if (cpy(dbs[parent].dirs[i].name, content, PATH_SIZE))
				{
					dbs[parent].dirs[i].inode = nInode;
					return true;
				}
				else
					return false;
			}
		}
		return false;
	}
	bool initDirBlock(int nInode, int nBlock, int parent, const string &content, const string &parent_name)
	{
		if (content.size() >= PATH_SIZE || parent_name.size() >= PATH_SIZE)
			return false;
		dbs[nBlock].dirs[14].inode = nInode;
		cpy(dbs[nBlock].dirs[14].name, content, PATH_SIZE);
		dbs[nBlock].dirs[15].inode = parent;
		cpy(dbs[nBlock].dirs[15].name, parent_name, PATH_SIZE);
		return true;
	}

	bool getAbsPath(string &content)
	{
		if (!content.empty())
		{
			if (content[0] == '.')
			{
				DirBlock &db = dbs[inodes[curr].block];
				if (content.size() >= 2 && content[1] == '.')
				{
					if (curr == 0)
						return false;
					string parent = db.dirs[15].name;
					content.replace(0, 2, parent);
				}
				else
				{
					string self = db.dirs[14].name;
					content.replace(0, 1, self);
				}
			}
		}
		return true;
	}

	void resize(int inode,int delta)
	{
		Inode &in = inodes[inode];
		in.sz += delta;
		if (inode != 0)//not root
			resize(in.mode == DIR ? dbs[in.block].p : fbs[in.block].p, delta);
	}
public:
	FileManeger(const string &_path):path(_path)
	{
		FILE* f = fopen(_path.c_str(), "rb");
		if (!f)
		{
			init();
			f = fopen(_path.c_str(), "rb");
		}
		for (int i = 0; i < MAX_SIZE; ++i)
			fread(f, inodeBitmap[i]);
		for (int i = 0; i < MAX_SIZE; ++i)
			fread(f, blockBitmap[i]);
		
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			inodes[i].read(f);
			if (inodes[i].mode != EMPTY)
			{
				blockToInode[inodes[i].block] = inodes[i].id;
			}
		}
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			if (blockToInode.count(i))//not empty
			{
				switch (inodes[blockToInode[i]].mode)
				{
				case FIL:
					fread(f,fbs[i].data);
					break;
				case DIR:
					for (int j = 0; j < 16; ++j)
					{
						fread(f, dbs[i].dirs[j].name);
						fread(f, dbs[i].dirs[j].inode);
					}
					break;
				default:
					break;
				}
			}
			else//empty
				fseek(f, MAX_SIZE, SEEK_CUR);
		}

		for (int i = 0; i < MAX_SIZE; ++i)
		{
			Inode &in = inodes[i];
			if (in.mode == DIR)
			{
				DirBlock &db = dbs[in.block];
				for (int j = 0; j < 14; ++j)
				{
					DirEntry &e = db.dirs[j];
					string name = db.dirs[j].name;
					if (!name.empty())
					{
						if (inodes[e.inode].mode == FIL)
							fbs[inodes[e.inode].block].p = i;
						else
							dbs[inodes[e.inode].block].p = i;
					}
				}
			}
		}
		fclose(f);
	}
	~FileManeger()
	{
		FILE *f = fopen(path.c_str(), "wb");
		write(f);
		fclose(f);
	}
	string exec(string opt,string content,string str)
	{
		if (!ALL_COMMAND.count(opt))
			return opt + " invalid command\n";
		if (!getAbsPath(content))
			return "root path has no parent\n";

		string ret;
		pii loc = nameToInode(content);//location
		if (REQUIRE_EXIST.count(opt)&&loc.first==-1)
		{
			return content + NON_EXIST;
		}

		int in=loc.first;
		int parent=loc.second;
		string sub;
		if (REQUIRE_PARENT.count(opt))
		{
			sub = content.substr(0, content.find_last_of('/'));
			if (parent==-1)
				return sub + NON_DIR;
		}
		else//存在->没问题 or 不要求存在->要求parent存在->不会进入这里
		{
			if (REQUIRE_DIR.count(opt))
			{
				if (inodes[in].mode != DIR)
					return content + NON_DIR;
			}
			if (REQUIRE_FILE.count(opt))
			{
				if (inodes[in].mode != FIL)
					return content + NON_FILE;
			}
		}

		if (opt == "pwd")
		{
			ret += print(curr, 0, false);
		}
		if (opt == "cd")
		{
			curr = in;
		}
		if (opt == "mkdir")
		{
			if (in!=-1)
				return content + ALREADY_EXIST;
			vector<string> names = split(content, "/");
			if (names.back().size() >= PATH_SIZE)
			{
				return content + TOO_LONG;
			}
			int nInode = allocateInode(DIR);
			int nBlock = allocateBlock(parent, nInode, DIR);
			initDirBlock(nInode, nBlock, parent, names[names.size()-1], names[names.size()-2]);
			allocateEntry(parent, nInode, names[names.size() - 1]);
		}
		if (opt == "ls"||opt=="ls_r")
		{
			if (inodes[in].mode == DIR)
			{
				ret += content + " is a directory\n";
				ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
				ret += print(in, 0, opt == "ls_r");
			}
			else
			{
				ret += content + " is a file\n";
				ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
				ret += fbs[inodes[in].block].data + string("\n");
			}
		}
		if (opt == "rmdir" || opt=="rm")
		{
			remove(in);
			resize(in, -inodes[in].sz);
			if (in == curr)
				curr = 0;
			ret += content + " removed\n";
		}
		if (opt == "echo")
		{

			if (in!=-1)
			{
				string chk=check("", -1, 0, ceil(1.0*str.size() / MAX_SIZE) - 1);
				if (!chk.empty())
					return chk;
				int old_sz=inodes[in].sz;
				cpy(fbs[inodes[in].block].data, str, MAX_SIZE);
				resize(in, (int)str.size() - old_sz);
			}
			else
			{
				//找空余的inode，空余的block，空余的dirs，修改map
				vector<string> names = split(content, "/");
				string chk = check(names.back(), parent, 1, ceil(1.0*str.size() / MAX_SIZE));
				if (!chk.empty())
					return chk;

				int nInode = allocateInode(FIL);
				int nBlock = allocateBlock(parent, nInode, FIL);
				allocateEntry(parent, nInode, names.back());
				cpy(fbs[nBlock].data, str,MAX_SIZE);
				resize(nInode, str.size());
			}
		}
		if (opt == "cat")
		{
			ret += fbs[inodes[in].block].data + string("\n");
		}
		if (opt == "write")
		{
			FILE *f = fopen(path.c_str(), "wb");
			write(f);
			fclose(f);
		}
		if (opt == "pwd_r")
		{
			ret += print(curr,0,true);
		}
		if (opt == "cpy")//cpy a b means b to a
		{
			if (!getAbsPath(str))
				return "root path has no parent\n";
			pii strloc = nameToInode(str);//location of str
			int strin = strloc.first, strp = strloc.second;
			if (strin!=-1)
			{
				if (inodes[strin].mode != FIL)
					return str + NON_FILE;
				if (content == str)
					return "destination and source is the same file\n";
				string sourStr = fbs[inodes[strin].block].data;
				if (in!=-1)
				{
					int old_sz = inodes[in].sz;
					if (!cpy(fbs[inodes[in].block].data, sourStr,MAX_SIZE))
					{
						return sourStr + TOO_LONG;
					}
					resize(in, (int)sourStr.size() - old_sz);
				}
				else
				{
					if (sourStr.size()>=MAX_SIZE)
					{
						return sourStr + TOO_LONG;
					}
					int nInode = allocateInode(FIL);
					int nBlock = allocateBlock(parent, nInode, FIL);
					vector<string> names = split(content, "/");
					allocateEntry(parent, nInode, names.back());
					cpy(fbs[nBlock].data, sourStr, MAX_SIZE);
					resize(nInode, sourStr.size());
				}
			}
			else
				return str + NON_EXIST;
		}
		if (opt == "cpydir")
		{
			if (!getAbsPath(content))
				return "root path has no parent\n";
			pii strloc = nameToInode(str);//location of str
			int strin = strloc.first, strp = strloc.second;
			if (strin!=-1)
			{
				if (inodes[strin].mode != DIR)
					return str + NON_DIR;
				if (in!=-1)
					return content + ALREADY_EXIST;
				int nInode = allocateInode(DIR);
				int nBlock = allocateBlock(parent, nInode, DIR);
				vector<string> names = split(content, "/");
				initDirBlock(nInode, nBlock, parent, names[content.size()-1], names[content.size() - 2]);
				allocateEntry(parent, nInode, names[content.size() - 1]);
				cpy(nInode, strin);
				resize(nInode, inodes[strin].sz);
			}
			else
				return str + NON_EXIST;
		}
		if (opt == "find")
		{
			ret=find(0, content, nullptr);
			if (ret.empty())
				ret = "no result found\n";
		}
		if (opt == "find_re")
		{
			regex *re = new regex(content);
			ret = find(0, content, re);
			delete re;
			if (ret.empty())
				ret = "no result found\n";
		}
		return ret;
	}
};

const string FileManeger::NON_EXIST = " No such file or directory\n";
const string FileManeger::NON_FILE = " is not a file\n";
const string FileManeger::NON_DIR = " is not a directory\n";
const string FileManeger::IS_EMPTY = " is empty\n";
const string FileManeger::ALREADY_EXIST = " already exists\n";
const string FileManeger::TOO_LONG = " directory name is too long\n";
const string FileManeger::FULL_DIR = " this dir is full\n";
const string FileManeger::FULL_SYS = " the system doesn't have enoug capacity\n";

const set<string> FileManeger::REQUIRE_DIR = {"cd","rmdir","ls_r"};
const set<string> FileManeger::REQUIRE_FILE = {"cat","rm"};
const set<string> FileManeger::REQUIRE_EXIST = { "cd","ls","rmdir","cat","rm","ls_r"};
const set<string> FileManeger::REQUIRE_PARENT = {"echo","mkdir","cpy","cpydir"};
const set<string> FileManeger::ALL_COMMAND = { "pwd","cd","mkdir","ls","ls_r","rmdir","echo","cat","rm","pwd_r","cpy","cpydir","find","find_re"};

