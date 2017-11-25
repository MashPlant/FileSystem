#pragma once
#include <string>
#include <iostream>
#include <cstdio>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
using namespace std;

//todo
//排序
//复制

class FileManeger
{
private:
	const static int MAX_SIZE = 4096;

	const static int FIL = 0;
	const static int DIR = 1;
	const static int EMPTY = 2;

	const static string NON_EXIST;
	const static string NON_FILE;
	const static string NON_DIR;

	const static set<string> REQUIRE_FILE;
	const static set<string> REQUIRE_DIR;
	const static set<string> REQUIRE_EXIST;
	const static set<string> REQUIRE_PARENT;

	bool inodeBitmap[MAX_SIZE];
	bool blockBitmap[MAX_SIZE];
	
	int curr = 0;//当前所在文件夹的inode

	struct Inode
	{
		int id;
		int mode;
		int sz;
		int block;
		void read(FILE *f)
		{
			fscanf(f, "%d%d%d%d", &id, &mode, &sz, &block);
		}
	};
	struct FileBlock
	{
		char data[MAX_SIZE] = { '\0' };
		int p;//指向父亲inode
	};

	struct DirEntry
	{
		char name[252] = {'\0'};
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
	//a little waste of memory

	Inode inodes[MAX_SIZE];
	string path;

	map<string, int> nameToInode;//todo 删除时在这里也要删
	//todo 文件名到inode也要有映射

	map<int, int> blockToInode;
	//no need of inode to block ,for Inode has field "block" recording it

	char encode(bool *beg,int len)//[)
	{
		int d = 0;
		for (int i=0; i<len; ++i)
		{
			d |= beg[i] << (len - 1 - i);
		}
		return (char)d;
	}
	void decode(char c, bool *beg, int len)//[)
	{
		int d = (int)c;
		for (int i = 0; i < len; ++i)
		{
			beg[i] = (1 << (len - 1 - i))&d;
		}
	}
	void cpy(char *dest, const string &str)
	{
		memcpy(dest, str.c_str(), str.size() * sizeof(char));
		dest[str.size()] = '\0';
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
					nameToInode.erase(p.dirs[i].name);
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
						nameToInode.erase(p.dirs[i].name);
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
	string print(int inode,int shift)
	{
		string ret, space;
		for (int i = 0; i < shift; ++i)
			space += "   ";
		Inode &in = inodes[inode];
		DirBlock &db = dbs[in.block];
		for (int i = 0; i < 14; ++i)
		{
			string child = db.dirs[i].name;
			if (!child.empty())
			{
				Inode &chi = inodes[db.dirs[i].inode];
				ret += space + child + "\n";
				if (chi.mode == DIR)
				{
					DirBlock &chb = dbs[chi.block];
					for (int j = 0; j < 14; ++j)
					{
						if (chb.dirs[j].name[0] != '\0')
						{
							ret += "   " + space + chb.dirs[j].name + "\n";
							ret +=print(chb.dirs[j].inode, shift + 2);
						}
					}
				}
			}
		}
		return ret;
	}
	void init()
	{
		FILE *f = fopen(path.c_str(), "w");
		//superblock
		for (int j = 0; j < 2; ++j)
		{
			fputc(1<<7, f);
			for (int i = 8; i < MAX_SIZE; i+=8)
			{
				fputc(0, f);
			}
		}
		//inode
		fprintf(f, "%d %d %d %d ", 0, DIR, 0, 0);
		for (int i = 1; i < MAX_SIZE; ++i)
		{
			fprintf(f, "%d %d %d %d ", 0, EMPTY, 0, 0);
		}
		//block
		fseek(f, MAX_SIZE*MAX_SIZE, SEEK_CUR);
		fprintf(f, "%d", EOF);
		fclose(f);
	}
	void write(FILE *f)
	{
		//superblock
		for (int i = 0; i < MAX_SIZE; i += 8)
		{
			char c = encode(inodeBitmap + i * 8, 8);
			fputc(c, f);
		}
		for (int i = 0; i < MAX_SIZE; i += 8)
		{
			char c = encode(blockBitmap + i * 8, 8);
			fputc(c, f);
		}
		//inode
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			Inode &in = inodes[i];
			fprintf(f, "%d %d %d %d ", in.id, in.mode, in.sz, in.block);
		}
		//block
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			if (blockToInode.count(i))
			{
				if (inodes[blockToInode[i]].mode == DIR)
				{
					DirBlock &db = dbs[i];
					for (int j = 0; j < 16; ++j)
					{
						for (int k = 0; k < 252; ++k)
						{
							if (!db.dirs[j].name[k])
							{
								fseek(f, 252 - k, SEEK_CUR);
								break;
							}
							fputc(db.dirs[j].name[k], f);
						}
						fprintf(f, "%d", db.dirs[j].inode);
					}
				}
				else
				{
					FileBlock &fb = fbs[i];
					for (int j = 0; j < MAX_SIZE; ++j)
					{
						if (!fb.data[j])
						{
							fseek(f, MAX_SIZE - j, SEEK_CUR);
							break;
						}
						fputc(fb.data[j], f);
					}
				}
			}
			else
				fseek(f, MAX_SIZE, SEEK_CUR);
		}
		fprintf(f, "%d", EOF);
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
	void allocateEntry(int parent,int nInode,const string &content)
	{
		for (int i = 0; i < 14; ++i)
		{
			if (dbs[parent].dirs[i].name[0] == '\0')
			{
				cpy(dbs[parent].dirs[i].name, content);
				dbs[parent].dirs[i].inode = nInode;
				break;
			}
		}
	}
	void initDirBlock(int nInode, int nBlock, int parent, const string &content, const string &sub)
	{
		dbs[nBlock].dirs[14].inode = nInode;
		cpy(dbs[nBlock].dirs[14].name, content);
		dbs[nBlock].dirs[15].inode = parent;
		cpy(dbs[nBlock].dirs[15].name, sub);
	}
public:
	FileManeger(const string &_path):path(_path)
	{
		FILE* f = fopen(_path.c_str(), "r");
		if (!f)
		{
			init();
			f = fopen(_path.c_str(), "r");
		}
		for (int i = 0; i < MAX_SIZE;i+=8)
		{
			char c = fgetc(f);
			decode(c, inodeBitmap + i * 8, 8);
		}
		for (int i = 0; i < MAX_SIZE; i += 8)
		{
			char c = fgetc(f);
			decode(c, blockBitmap + i * 8, 8);
		}
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			inodes[i].read(f);
			if (inodes[i].mode != EMPTY)
			{
				blockToInode[inodes[i].block] = inodes[i].id;
			}
		}
		fgetc(f);//多余空格
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			if (blockToInode.count(i))//not empty
			{
				switch (inodes[blockToInode[i]].mode)
				{
				case FIL:
					for (int j = 0; j < MAX_SIZE; ++j)
					{
						char c = fgetc(f);
						fbs[i].data[j] = c;
					}
					break;
				case DIR:
					for (int j = 0; j < 16; ++j)
					{
						for (int k = 0; k < 252; ++k)
						{
							char c = fgetc(f);
							dbs[i].dirs[j].name[k] = c;
						}
						int d;
						fscanf(f, "%d", &d);
						dbs[i].dirs[j].inode = d;
					}
					break;
				default:
					break;
				}
			}
			else//empty
			{
				fseek(f, MAX_SIZE, SEEK_CUR);
			}
		}
		nameToInode[""] = 0;//根目录
		nameToInode["/"] = 0;
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
						nameToInode[name] = e.inode;
						if (inodes[e.inode].mode == FIL)
						{
							fbs[inodes[e.inode].block].p = i;
						}
						else
						{
							dbs[inodes[e.inode].block].p = i;
						}
					}
				}
			}
		}
		fclose(f);
	}
	~FileManeger()
	{
		FILE *f = fopen(path.c_str(), "w");
		write(f);
		fclose(f);
	}
	string exec(string opt,string content,string str)
	{
		string ret;
		if (!content.empty())
		{
			if (content[0] == '.')
			{
				DirBlock &db = dbs[inodes[curr].block];
				if (content.size() >= 2 && content[1] == '.')
				{
					if (curr == 0)
					{
						ret += "root path has no parent\n";
						return ret;
					}
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
		if (REQUIRE_EXIST.count(opt))
		{
			if (!nameToInode.count(content))
				return content + NON_EXIST;
		}

		int in;
		string sub;
		int parent;
		if (REQUIRE_PARENT.count(opt))
		{
			sub = content.substr(0, content.find_last_of('/'));
			if (!nameToInode.count(sub))
				return sub + NON_DIR;
			parent = nameToInode[sub];
		}
		else//存在->没问题 or 不要求存在->要求parent存在->不会进入这里
		{
			in = nameToInode[content];
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
			Inode &in = inodes[curr];
			DirBlock &db = dbs[in.block];
			for (int i = 0; i < 14; ++i)
			{
				string child = db.dirs[i].name;
				if (!child.empty())
					ret += child + '\n';
			}
			if (ret.empty())
			{
				ret += "curr dir is now empty\n";
			}
		}
		if (opt == "cd")
		{
			curr = in;
		}
		if (opt == "mkdir")
		{
			if (nameToInode.count(content))
			{
				ret += content + " already exists\n";
				return ret;
			}
			int nInode = allocateInode(DIR);
			nameToInode[content] = nInode;
			int nBlock = allocateBlock(parent, nInode, DIR);
			initDirBlock(nInode, nBlock, parent, content, sub);
			allocateEntry(parent, nInode, content);
		}
		if (opt == "ls")
		{
			if (inodes[in].mode == DIR)
				ret += content + " is a directory\n";
			else
				ret += content + " is a file\n";
		}
		if (opt == "rmdir")
		{
			remove(in);
			ret += content + " removed\n";
		}
		if (opt == "echo")
		{
			if (nameToInode.count(content))
			{
				int old = nameToInode[content];
				cpy(fbs[inodes[old].block].data, content);
			}
			else
			{
				//四步:找空余的inode，空余的block，空余的dirs，修改map
				int nInode = allocateInode(FIL);
				nameToInode[content] = nInode;
				int nBlock = allocateBlock(parent, nInode, FIL);
				allocateEntry(parent, nInode, content);
			}
		}
		if (opt == "cat")
		{
			ret += fbs[inodes[in].block].data + string("\n");
		}
		if (opt == "rm")
		{
			remove(in);
			ret += content+" removed\n";
		}
		if (opt == "write")
		{
			FILE *f = fopen(path.c_str(), "w");
			write(f);
			fclose(f);
		}
		if (opt == "pwd_r")
		{
			ret += print(in,0);
		}
		return ret;
	}
};

const string FileManeger::NON_EXIST = " No such FILE or directory\n";
const string FileManeger::NON_FILE = " is not a file\n";
const string FileManeger::NON_DIR = " is not a directory\n";
const set<string> FileManeger::REQUIRE_DIR = {"cd","rmdir","pwd_r"};
const set<string> FileManeger::REQUIRE_FILE = {"cat","rm"};
const set<string> FileManeger::REQUIRE_EXIST = { "cd","ls","rmdir","cat","rm","pwd_r"};
const set<string> FileManeger::REQUIRE_PARENT = {"echo","mkdir","cpy"};
