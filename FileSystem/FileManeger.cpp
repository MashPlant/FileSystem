#include "stdafx.h"
#include "FileManeger.h"
using namespace std;
const string FileManeger::NON_EXIST = " No such file or directory\n";
const string FileManeger::NON_FILE = " is not a file\n";
const string FileManeger::NON_DIR = " is not a directory\n";
const string FileManeger::IS_EMPTY = " is empty\n";
const string FileManeger::ALREADY_EXIST = " already exists\n";
const string FileManeger::TOO_LONG = " directory name is too long\n";
const string FileManeger::FULL_DIR = " this dir is full\n";
const string FileManeger::FULL_SYS = " the system doesn't have enoug capacity\n";

const set<string> FileManeger::REQUIRE_DIR = { "cd","rmdir","ls_r" };
const set<string> FileManeger::REQUIRE_FILE = { "cat","rm","append" };
const set<string> FileManeger::REQUIRE_EXIST = { "cd","ls","rmdir","cat","rm","ls_r","append" };
const set<string> FileManeger::REQUIRE_PARENT = { "echo","mkdir","cpy","cpydir" };
const set<string> FileManeger::ALL_COMMAND = { "write","pwd","cd","mkdir","ls","ls_r","rmdir","echo","cat","rm","pwd_r","cpy","cpydir","find","find_re","append" };


inline int FileManeger::getP(int inode)
{
	return inodes[inode].p;
}

inline int FileManeger::getMode(int inode)
{
	return inodes[inode].mode;
}

inline string FileManeger::getData(int inode)
{
	string ret;
	for (int i = 0; i < inodes[inode].cnt; ++i)
	{
		ret += fbs[inodes[inode].block + i].data;
	}
	return ret;
}

inline string FileManeger::getName(int inode)
{
	if (getMode(inode) == DIR)
		return dbs[inodes[inode].block].dirs[14].name;
	if (getMode(inode) == FIL)
	{
		int p = getP(inode);
		DirBlock &db = dbs[p];
		for (int i = 0; i<14; ++i)
		{
			if (db.dirs[i].inode == inode)
				return db.dirs[i].name;
		}
	}
	return "";
}

string FileManeger::check(const string &name, int parent, int requiredInodes, int requiredBlocks)
{
	if (name.size() >= PATH_SIZE)
		return TOO_LONG;
	if (parent != -1)//不要求检查parent
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

FileManeger::pii FileManeger::nameToInode(const string &name)//first:self,second:parent
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
	for (int i = 1; i < names.size(); ++i)
	{
		bool found = false;
		for (int j = 0; j < 14; ++j)
		{
			if (getMode(self) == FIL)//如果真的要找文件，只可能在最后一项出现文件，此时i循环已退出
				return make_pair(-1, -1);
			string child = dbs[inodes[self].block].dirs[j].name;
			if (!child.empty() && child == names[i])
			{
				self = dbs[inodes[self].block].dirs[j].inode;
				if (i + 1 != names.size())
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

void FileManeger::remove(int inode)
{
	if (getMode(inode) == FIL)
	{
		Inode &in = inodes[inode];
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			for (int i = 0; i<in.cnt; ++i)
			{
				blockBitmap[inodes[inode].block + i] = false;//标记即可，不用真的清零
				blockToInode.erase(inodes[inode].block + i);
			}
		}
		FileBlock &fb = fbs[inodes[inode].block];
		DirBlock &p = dbs[inodes[getP(inode)].block];
		for (int i = 0; i < 14; ++i)
			if (p.dirs[i].inode == inode)
			{
				memset(p.dirs[i].name, 0, sizeof(p.dirs[i].name));
				break;
			}
	}
	else if (getMode(inode) == DIR)
	{
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			blockBitmap[inodes[inode].block] = false;
		}
		DirBlock &db = dbs[inodes[inode].block];
		if (inode != 0)
		{
			DirBlock &p = dbs[inodes[getP(inode)].block];
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

void FileManeger::cpy(int dest, int sour)
{
	if (getMode(sour) == DIR)
	{
		DirBlock &ddb = dbs[inodes[dest].block];
		DirBlock &sdb = dbs[inodes[sour].block];
		for (int i = 0; i < 14; ++i)
		{
			string name = sdb.dirs[i].name;
			if (!name.empty())
			{
				string parentName = ddb.dirs[14].name;
				if (getMode(sdb.dirs[i].inode) == DIR)
					cpy(allocator.createDir(dest, name, parentName), sdb.dirs[i].inode);
				else
				{
					const string& source = getData(sdb.dirs[i].inode);
					int nInode = allocator.createFile(dest, name, source);
					inodes[nInode].sz = source.size();
				}
			}
		}
	}
}

string FileManeger::print(int inode, int shift, bool r)
{
	string ret, space;
	for (int i = 0; i < shift; ++i)
		space += "   ";
	if (getMode(inode) == FIL)
		return "";
	Inode &in = inodes[inode];
	DirBlock &db = dbs[in.block];
	for (int i = 0; i < 14; ++i)
	{
		string child = db.dirs[i].name;
		if (!child.empty())
		{
			ret += space + (getMode(db.dirs[i].inode) == DIR ? "/" : "") + child + '\n';
			if (r)
				ret += print(db.dirs[i].inode, shift + 1, r);
		}
	}
	return ret;
}
string FileManeger::find(int inode, const string &match, regex *re)
{
	string ret;
	if (getMode(inode) == FIL)
		return "";
	Inode &in = inodes[inode];
	DirBlock &db = dbs[in.block];
	for (int i = 0; i < 14; ++i)
	{
		string child = db.dirs[i].name;
		if (!child.empty())
		{
			if (re ? regex_search(child, *re) : child.find(match) != string::npos)
				ret += (getMode(db.dirs[i].inode) == DIR ? "/" : "") + child + '\n';
			ret += find(db.dirs[i].inode, match, re);
		}
	}
	return ret;
}

void FileManeger::init()
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
void FileManeger::write(FILE *f)
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
			if (getMode(blockToInode[i]) == DIR)
				for (int j = 0; j < 16; ++j)
					fwrite(f, dbs[i].dirs[j].name, dbs[i].dirs[j].inode);
			else
				fwrite(f, fbs[i].data);
		}
		else
			writeEmpty(f, MAX_SIZE);
	}
}

bool FileManeger::getAbsPath(string &content)
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
				string parent = (db.dirs[15].name[0] ? "/" : "") + string(db.dirs[15].name);
				content.replace(0, 2, parent);
				string content1 = content;
			}
			else
			{
				string self = (db.dirs[14].name[0] ? "/" : "") + string(db.dirs[14].name);
				content.replace(0, 1, self);
				//string content1 = content;
			}
		}
	}
	return true;
}

void FileManeger::resize(int inode, int delta)
{
	Inode &in = inodes[inode];
	in.sz += delta;
	if (inode != 0)//not root
		resize(getP(inode), delta);
}

FileManeger::pii FileManeger::count(int inode)//first:inode, second block
{
	int kind = getMode(inode);
	if (kind == FIL)
	{
		return make_pair(1, inodes[inode].cnt);
	}
	if (kind == DIR)
	{
		pii ret(0, 0);
		for (int i = 0; i<14; ++i)
		{
			if (dbs[inodes[inode].block].dirs[i].name[0] != '\0')
			{
				pii temp = count(dbs[inodes[inode].block].dirs[i].inode);
				ret.first += temp.first;
				ret.second += temp.second;
			}
		}
		return ret;
	}
	return make_pair(0, 0);
}

FileManeger::FileManeger(const string &_path) :path(_path)
{
	allocator = Allocator(this);
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
		if (getMode(i) == FIL)
		{
			for (int j = 0; j < inodes[i].cnt; ++j)
				blockToInode[inodes[i].block + j] = inodes[i].id;
		}
		else
			blockToInode[inodes[i].block] = inodes[i].id;
	}
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		if (blockToInode.count(i))//not empty
		{
			switch (getMode(blockToInode[i]))
			{
			case FIL:
				fread(f, fbs[i].data);
				break;
			case DIR:
				for (int j = 0; j < 16; ++j)
					fread(f, dbs[i].dirs[j].name, dbs[i].dirs[j].inode);
				break;
			default:
				break;
			}
		}
		else//empty
			fseek(f, MAX_SIZE, SEEK_CUR);
	}
	fclose(f);
}
FileManeger::~FileManeger()
{
	FILE *f = fopen(path.c_str(), "wb");
	write(f);
	fclose(f);
}

string FileManeger::exec(string opt, string content, string str)
{
	if (!ALL_COMMAND.count(opt))
		return opt + " invalid command\n";
	if (!getAbsPath(content))
		return "root path has no parent\n";

	string ret;
	pii loc = nameToInode(content);//location
	if (REQUIRE_EXIST.count(opt) && loc.first == -1)
	{
		return content + NON_EXIST;
	}
	int in = loc.first;
	int parent = loc.second;
	string sub;
	if (REQUIRE_PARENT.count(opt))
	{
		sub = content.substr(0, content.find_last_of('/'));
		if (parent == -1)
			return sub + NON_DIR;
	}
	else//存在->没问题 or 不要求存在->要求parent存在->不会进入这里
	{
		if (REQUIRE_DIR.count(opt))
		{
			if (getMode(in) != DIR)
				return content + NON_DIR;
		}
		if (REQUIRE_FILE.count(opt))
		{
			if (getMode(in) != FIL)
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
		if (in != -1)
			return content + ALREADY_EXIST;
		vector<string> names = split(content, "/");
		string chk = check(names.back(), -1, 0, 0);
		if (!chk.empty())
			return chk;
		allocator.createDir(parent, names[names.size() - 1], names[names.size() - 2]);
	}
	if (opt == "ls" || opt == "ls_r")
	{
		if (getMode(in) == DIR)
		{
			ret += content + " is a directory\n";
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
			ret += print(in, 0, opt == "ls_r");
		}
		else
		{
			ret += content + " is a file\n";
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
			ret += getData(in) + string("\n");
		}
	}
	if (opt == "rmdir" || opt == "rm")
	{
		remove(in);
		resize(in, -inodes[in].sz);
		if (in == curr)
			curr = 0;
		ret += content + " removed\n";
	}
	if (opt == "echo")
	{
		if (in != -1)
		{
			if (getMode(in) != FIL)
				return content + NON_FILE;
			string chk = check("", -1, 0, ceil(1.0*str.size() / MAX_SIZE));
			if (!chk.empty())
				return chk;
			string name = getName(in);
			remove(in);
			resize(in, -inodes[in].sz);
			if (str == "\"\"")
				str.clear();
			int nInode = allocator.createFile(parent, name, str);
			resize(nInode, str.size());
		}
		else
		{
			//找空余的inode，空余的block，空余的dirs，修改map
			vector<string> names = split(content, "/");
			string chk = check(names.back(), parent, 1, ceil(1.0*str.size() / MAX_SIZE));
			if (!chk.empty())
				return chk;
			if (str == "\"\"")
				str.clear();
			int nInode = allocator.createFile(parent, names.back(), str);
			resize(nInode, str.size());
		}
	}
	if (opt == "cat")
	{
		for (int i = 0; i<inodes[in].cnt; ++i)
		{
			ret += fbs[inodes[in].block + i].data;
		}
		ret += '\n';
	}
	if (opt == "write")
	{
		FILE *f = fopen(path.c_str(), "wb");
		write(f);
		fclose(f);
	}
	if (opt == "pwd_r")
	{
		ret += print(curr, 0, true);
	}
	//考虑到inode和block可能用尽，copy操作时也进行检查
	if (opt == "cpy")//cpy a b means b to a
	{
		if (!getAbsPath(str))
			return "root path has no parent\n";
		pii sourceloc = nameToInode(str);//location of source
		int sourcein = sourceloc.first, sourcep = sourceloc.second;
		if (sourcein != -1)
		{
			if (getMode(sourcein) != FIL)
				return str + NON_FILE;
			if (content == str)
				return "destination and source is the same file\n";
			string source;
			for (int i = 0; i < inodes[sourcein].cnt; ++i)
				source += fbs[inodes[sourcein].block + i].data;
			return exec("echo", content, source);
		}
		else
			return str + NON_EXIST;
	}
	if (opt == "cpydir")
	{
		if (!getAbsPath(content))
			return "root path has no parent\n";
		pii sourceloc = nameToInode(str);
		int sourcein = sourceloc.first, sourcep = sourceloc.second;
		if (sourcein != -1)
		{
			if (getMode(sourcein) != DIR)
				return str + NON_DIR;
			if (in != -1)
				return content + ALREADY_EXIST;
			pii cnt = count(sourcein);
			vector<string> names = split(content, "/");
			string chk = check(names.back(), parent, cnt.first, cnt.second);
			if (!chk.empty())
				return chk;
			int nInode = allocator.createDir(parent, names[names.size() - 1], names[names.size() - 2]);
			//保证申请到了一片连续的inode和block，最坏情况下也能保证复制完成
			cpy(nInode, sourcein);
			resize(nInode, inodes[sourcein].sz);
		}
		else
			return str + NON_EXIST;
	}
	if (opt == "find")
	{
		ret = find(0, content, nullptr);
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
	if (opt == "append")
	{
		//删文件重写，暴力出奇迹
		string source = getData(in) + str;
		string chk = check("", -1, 0, ceil(1.0 * source.size() / MAX_SIZE));
		if (!chk.empty())
			return chk;
		string name = getName(in);
		remove(in);
		resize(in, -inodes[in].sz);
		if (str == "\"\"")
			str.clear();
		int nInode = allocator.createFile(parent, name, source);
		resize(nInode, source.size());
	}
	return ret;
}

int FileManeger::Allocator::allocateInode(int mode, int p)
{
	int nInode = -1;
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		if (!self->inodeBitmap[i])
		{
			self->inodeBitmap[i] = true;
			self->inodes[i].id = i;
			self->inodes[i].mode = mode;
			self->inodes[i].sz = 0;
			self->inodes[i].p = p;
			nInode = i;
			break;
		}
	}
	return nInode;
}

int FileManeger::Allocator::allocateBlock(int parent, int nInode, int mode, int size)
{
	int nBlock = -1;
	for (int i = 0; i <= MAX_SIZE - size; ++i)
	{
		bool free = true;
		for (int j = 0; j < size; ++j)
		{
			if (self->blockBitmap[i + j])
			{
				free = false;
				break;
			}
		}
		if (!free)
			continue;
		//这里保证申请到了一块连续的block，起始点为i，长度为size，而且都已经赋予好初值了
		for (int j = 0; j<size; ++j)
		{
			self->blockToInode[i + j] = nInode;
			self->blockBitmap[i + j] = true;
		}
		self->inodes[nInode].block = i;
		self->inodes[nInode].cnt = size;
		nBlock = i;
		break;
	}
	return nBlock;
}
//约定所有的parent都是指parent的inode
void FileManeger::Allocator::allocateEntry(int parent, int nInode, const string &content)
{
	parent = self->inodes[parent].block;//此前inode与block一对一对应故而没有出问题
	for (int i = 0; i < 14; ++i)
		if (self->dbs[parent].dirs[i].name[0] == '\0')
		{
			::cpy(self->dbs[parent].dirs[i].name, content, PATH_SIZE);
			self->dbs[parent].dirs[i].inode = nInode;
			break;
		}
}

void FileManeger::Allocator::initDirBlock(int nInode, int nBlock, int parent, const string &content, const string &parent_name)
{
	self->dbs[nBlock].dirs[14].inode = nInode;
	::cpy(self->dbs[nBlock].dirs[14].name, content, PATH_SIZE);
	self->dbs[nBlock].dirs[15].inode = parent;
	::cpy(self->dbs[nBlock].dirs[15].name, parent_name, PATH_SIZE);
}

int FileManeger::Allocator::createFile(int parent, const string &name, string source)
{
	int nInode = allocateInode(FIL, parent);
	int nBlock = allocateBlock(parent, nInode, FIL, ceil(1.0*source.size() / MAX_SIZE));
	//一次性申请了一个区间内的block,如果source为空，不会影响除了inodes[nInode]之外的地方
	allocateEntry(parent, nInode, name);
	if (!source.empty())
		::cpy(self->fbs[nBlock].data, source, MAX_SIZE - 1);//考虑'\0'
	while (source.size() >= MAX_SIZE)
	{
		source = source.substr(MAX_SIZE - 1);
		::cpy(self->fbs[++nBlock].data, source, MAX_SIZE - 1);
	}
	return nInode;
}

int FileManeger::Allocator::createDir(int parent, const string &name, const string &parentName)
{
	int nInode = allocateInode(DIR, parent);
	int nBlock = allocateBlock(parent, nInode, DIR);
	initDirBlock(nInode, nBlock, parent, name, parentName);
	allocateEntry(parent, nInode, name);
	return nInode;
}