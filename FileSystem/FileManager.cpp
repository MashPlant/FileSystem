#include "stdafx.h"
#include "FileManager.h"
using namespace std;
const string FileManager::NON_EXIST = " No such file or directory\n";
const string FileManager::NON_FILE = " is not a file\n";
const string FileManager::NON_DIR = " is not a directory\n";
const string FileManager::NON_ZIP = " is not a zip\n";
const string FileManager::IS_EMPTY = " is empty\n";
const string FileManager::ALREADY_EXIST = " already exists\n";
const string FileManager::TOO_LONG = " directory name is too long\n";
const string FileManager::FULL_DIR = " this dir is full\n";
const string FileManager::FULL_SYS = " the system doesn't have enoug capacity\n";

const set<string> FileManager::REQUIRE_DIR = { "cd","rmdir" };
const set<string> FileManager::REQUIRE_FILE = { "cat","rm","append" };
const set<string> FileManager::REQUIRE_EXIST = { "cd","ls","rmdir","cat","rm","append" };
const set<string> FileManager::REQUIRE_PARENT = { "echo","mkdir","cpy","cpydir","zip" ,"dezip"};
const set<string> FileManager::REQUIRE_WRITE = { "echo" ,"mkdir" ,"rm" ,"rmdir" ,"cpy" ,"cpydir" ,"append" ,"zip","dezip" };

const map<string, int> FileManager::ALL_COMMAND =
{ {"time",1},{"exit",1},{"write",1},{"pwd",1},{"cd",2},{"mkdir",2},{"ls",2},{"rmdir",2},{"echo",3},{"cat",2},{"rm",2},{"cpy",3},{ "zip",3 },{ "dezip",3 },{"cpydir",3},{"find",2},{"append",3} };
inline FileManager::DirBlock& FileManager::getDBFromInode(int inode) { return blocks[inodes[inode].block].db; }
inline FileManager::DirBlock& FileManager::getDBFromIndex(int index) { return blocks[index].db; }
inline FileManager::FileBlock& FileManager::getFBFromInode(int inode) { return blocks[inodes[inode].block].fb; }
inline FileManager::FileBlock& FileManager::getFBFromIndex(int index) { return blocks[index].fb; }
inline int FileManager::getP(int inode) { return inodes[inode].p; }inline int FileManager::getMode(int inode) { return inodes[inode].mode; }

inline string FileManager::getData(int inode,bool full)
{
	string ret;
	for (int i = 0; i < inodes[inode].cnt; ++i)
		ret += full ? fullCharArrToStr(getFBFromIndex(inodes[inode].block + i).data) : getFBFromIndex(inodes[inode].block + i).data;
	return ret;
}

inline string FileManager::getName(int inode)
{
	if (getMode(inode) == DIR)
		return inode ? getDBFromInode(inode).dirs[14].name : "/";
	if (getMode(inode) == FIL|| getMode(inode) == ZIP)
	{
		DirBlock &db = getDBFromInode(getP(inode));
		for (int i = 0; i<14; ++i)
			if (db.dirs[i].inode == inode)
				return db.dirs[i].name;
	}
	return "";
}

FileManager::pii FileManager::nameToInode(const string &name)//first:self,second:parent
{
	if (name == "" || name == "/")
		return make_pair(0, -1);
	vector<string> names = split(name, "/");
	int self = 0, parent = 0;
	if (names[0] == ".")//除非输入空串，否则分割一定有结果
		self = curr, parent = getP(self);
	if (names[0] == "..")
		self = getP(curr), parent = getP(self);
	if (names[0] != "."&&names[0] != ".."&&names[0] != "")
		return make_pair(-1, -1);
	for (int i = 1; i < names.size(); ++i)
	{
		bool found = false;
		for (int j = 0; j < 14; ++j)
		{
			if (getMode(self) == FIL|| getMode(self) == ZIP)//如果真的要找文件，只可能在最后一项出现文件，此时i循环已退出
				return make_pair(-1, -1);
			string child = getDBFromInode(self).dirs[j].name;
			if (!child.empty() && child == names[i])
			{
				self = getDBFromInode(self).dirs[j].inode;
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

void FileManager::remove(int inode)
{
	if (getMode(inode) == FIL|| getMode(inode) == ZIP)
	{
		Inode &in = inodes[inode];
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			for (int i = 0; i<in.cnt; ++i)
			{
				blockBitmap[inodes[inode].block + i] = false;//标记即可，不用真的清零
				blockToInode[inodes[inode].block + i] = -1;
			}
		}
		inodes[getP(inode)].t = time(nullptr);
		DirBlock &p = getDBFromInode(getP(inode));
		for (int i = 0; i < 14; ++i)
			if (p.dirs[i].inode == inode)
			{
				memset(p.dirs[i].name, 0, sizeof(p.dirs[i].name));
				break;
			}
	}
	else if (getMode(inode) == DIR)
	{
		DirBlock &db = getDBFromInode(inode);
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			blockBitmap[inodes[inode].block] = false;
			blockToInode[inodes[inode].block] = -1;
			inodes[getP(inode)].t = time(nullptr);
			DirBlock &p = getDBFromInode(getP(inode));
			for (int i = 0; i < 14; ++i)
				if (p.dirs[i].inode == inode)
				{
					memset(p.dirs[i].name, 0, sizeof(p.dirs[i].name));
					break;
				}
		}
		for (int i = 0; i < 14; ++i)
			if (db.dirs[i].name[0] != '\0')
				remove(db.dirs[i].inode);
	}
}

void FileManager::cpy(int dest, int sour)
{
	if (getMode(sour) == DIR)
	{
		DirBlock &ddb = getDBFromInode(dest);
		DirBlock &sdb = getDBFromInode(sour);
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

string FileManager::print(int inode, int shift, bool r)
{
	string ret, space;
	for (int i = 0; i < shift; ++i)
		space += "   ";
	if (getMode(inode) == FIL|| getMode(inode) == ZIP)
		return "";
	DirBlock &db = getDBFromInode(inode);
	for (int i = 0; i < 14; ++i)
	{
		string child = db.dirs[i].name;
		if (!child.empty())
		{
			ret += space + "/" + child;
			ret += showTime ? '\t' + string(ctime(&inodes[db.dirs[i].inode].t)) : "\n";
			if (r)
				ret += print(db.dirs[i].inode, shift + 1, r);
		}
	}
	return ret;
}
string FileManager::find(int inode, const string &match, regex *re)
{
	string ret;
	if (getMode(inode) == FIL|| getMode(inode) == ZIP)
		return "";
	DirBlock &db = getDBFromInode(inode);
	for (int i = 0; i < 14; ++i)
	{
		string child = db.dirs[i].name;
		if (!child.empty())
		{
			child = "./" + child;
			getAbsPath(child,inode);
			if (re ? regex_search(child, *re) : child.find(match) != string::npos)
				ret += child + "\n";
			ret += find(db.dirs[i].inode, match, re);
		}
	}
	return ret;
}

void FileManager::init()
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
	empty.mode = EMPTY, dir.mode = DIR, dir.t = time(nullptr);
	dir.print(f);
	for (int i = 1; i < MAX_SIZE; ++i)
		empty.print(f);
	//block
	writeEmpty(f, MAX_SIZE*MAX_SIZE);
	fclose(f);
}
void FileManager::write(FILE *f)
{
	//clock_t beg = clock();
	//superblock
	for (int i = 0; i < MAX_SIZE; ++i)
		fwrite(f, inodeBitmap[i]);
	for (int i = 0; i < MAX_SIZE; ++i)
		fwrite(f, blockBitmap[i]);
	//std::cout << clock() - beg<<endl;
	//beg = clock();
	//inode
	for (int i = 0; i < MAX_SIZE; ++i)
		inodes[i].print(f);
	//std::cout << clock() - beg << endl;
	//clock_t empty_time = 0;
	//block
	int last = MAX_SIZE;
	for (; blockToInode[last - 1] == -1; --last);
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		if (i==last)
		{
			writeEmpty(f, MAX_SIZE*(MAX_SIZE - last));
			break;
		}
		if (blockToInode[i] != -1)
		{
			int mode = getMode(blockToInode[i]);
			switch (mode)
			{
			case DIR:
				for (int j = 0; j < 16; ++j)
					fwrite(f, getDBFromIndex(i).dirs[j].name, getDBFromIndex(i).dirs[j].inode);
				break;
			case ZIP:
			case FIL:
				fwrite(f, getFBFromIndex(i).data, mode == ZIP);
				
				break;
			}
		}
		else
			writeEmpty(f, MAX_SIZE);
	}	
	//std::cout << clock() - beg << endl;
	//beg = clock();
}

bool FileManager::getAbsPath(string &content,int _curr)
{
	if (_curr == -1)
		_curr = curr;
	if (!content.empty())
	{
		if (content[0] == '.')
		{
			DirBlock &db = getDBFromInode(_curr);
			if (content.size() >= 2 && content[1] == '.')//..
			{
				if (_curr == 0)
					return false;
				content.erase(0, 2);
				int p = db.dirs[15].inode;
				while(p)
				{
					content = getName(p) + (content.empty() ? "" : "/" + content);
					p = getDBFromInode(p).dirs[15].inode;
				}
				content = "/" + content;
			}
			else
			{
				content.erase(0, 1);
				int p = db.dirs[14].inode;
				while (p)
				{
					content = "/" + getName(p) + content;
					p = getDBFromInode(p).dirs[15].inode;
				}
				if (content.empty())
					content = "/";
			}
		}
	}
	return true;
}

void FileManager::resize(int inode, int delta)
{
	inodes[inode].sz += delta;
	if (inode != 0)//not root
		resize(getP(inode), delta);
}

FileManager::pii FileManager::count(int inode)//first:inode, second block
{
	int kind = getMode(inode);
	if (kind == FIL|| getMode(inode) == ZIP)
		return make_pair(1, inodes[inode].cnt);
	if (kind == DIR)
	{
		pii ret(0, 0);
		for (int i = 0; i < 14; ++i)
		{
			if (getDBFromInode(inode).dirs[i].name[0] != '\0')
			{
				pii temp = count(getDBFromInode(inode).dirs[i].inode);
				ret.first += temp.first;
				ret.second += temp.second;
			}
		}
		return ret;
	}
	return make_pair(0, 0);
}

FileManager::FileManager(const string &_path) :path(_path)
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
	memset(blockToInode, -1, sizeof(blockToInode));
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		inodes[i].read(f);
		if (getMode(i) == FIL || getMode(i) == ZIP)
		{
			for (int j = 0; j < inodes[i].cnt; ++j)
				blockToInode[inodes[i].block + j] = inodes[i].id;
		}
		else
			blockToInode[inodes[i].block] = inodes[i].id;
	}
	for (int i = 0; i < MAX_SIZE; ++i)
	{
		if (blockToInode[i] != -1)//not empty
		{
			switch (getMode(blockToInode[i]))
			{
			case ZIP:
			case FIL:
				fread(f, getFBFromIndex(i).data);
				break;
			case DIR:
				for (int j = 0; j < 16; ++j)
					fread(f, getDBFromIndex(i).dirs[j].name, getDBFromIndex(i).dirs[j].inode);
				break;
			}
		}
		else//empty
			writeEmpty(f, MAX_SIZE);
	}
	fclose(f);
}
FileManager::~FileManager()
{
	FILE *f;
	fopen_s(&f, path.c_str(), "wb");
	write(f);
	fclose(f);
}

string FileManager::exec(vector<string> command)
{
	if (command.empty())
		return	"";
	string opt = command[0], content, str;
	string parameter;
	if (command.size() >= 2)
	{
		content = command[1];
		if (command.size() >= 3)
			str = command[2];
		if (command.size() >= 4)
			parameter = command[3];
		if (opt == "echo" || opt == "append")
			swap(content, str);//看错题了，这样改比较快
	}

	if (!ALL_COMMAND.count(opt))
		return opt + " invalid command\n";
	if (ALL_COMMAND.at(opt) > command.size())//允许附加参数
		return "wrong arguments number\n";
	if (!getAbsPath(content))
		return "root path has no parent\n";

	string ret;
	pii loc = nameToInode(content);//location
	if (REQUIRE_EXIST.count(opt) && loc.first == -1)
		return content + NON_EXIST;
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
	if (opt == "time")
	{
		showTime = !showTime;
	}
	if (opt == "exit")
	{
		return "exited";
	}
	if (opt == "pwd")
	{
		ret += print(curr, 0, command.size() >= 2 && command[1] == "-r");
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
		string chk = allocator.check(names.back(), parent, 1, 1);
		if (!chk.empty())
			return chk;
		allocator.createDir(parent, names.back(), names[names.size() - 2]);
	}
	if (opt == "ls")
	{
		if (getMode(in) == DIR)
		{
			ret += content + " is a directory\t" + (showTime ? ctime(&inodes[in].t) : "\n");
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
			ret += getName(in) + "\n";
			ret += print(in, 1, command.size() >= 3 && command[2] == "-r");
		}
		else if (getMode(in) == FIL)
		{
			ret += content + " is a file\t" + (showTime ? ctime(&inodes[in].t) : "\n");
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
			ret += getData(in) + string("\n");
		}
		else if (getMode(in) == ZIP)
		{
			ret += content + " is a zip\t" + (showTime ? ctime(&inodes[in].t) : "\n");
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
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
		string name;
		if (in != -1)
		{
			if (getMode(in) != FIL)
				return content + NON_FILE;
			string chk = allocator.check("", -1, 0, ceil(1.0*str.size() / MAX_SIZE));
			if (!chk.empty())
				return chk;
			name = getName(in);
			remove(in);
			resize(in, -inodes[in].sz);
		}
		else
		{
			vector<string> names = split(content, "/");
			string chk = allocator.check(names.back(), parent, 1, ceil(1.0*str.size() / MAX_SIZE));
			if (!chk.empty())
				return chk;
			name = names.back();
		}
		if (str == "\"\"")
			str.clear();
		int nInode = allocator.createFile(parent, name, str, parameter == "z");
		resize(nInode, str.size());
	}
	if (opt == "cat")
	{
		ret = getData(in) + "\n";
	}
	if (opt == "write")
	{
		FILE *f;// = fopen(path.c_str(), "wb");
		fopen_s(&f, path.c_str(), "wb");
		write(f);
		fclose(f);
	}
	//考虑到inode和block可能用尽，copy操作时也进行检查
	if (opt == "cpy" || opt == "zip" || opt == "dezip")//cpy/zip/dezip a b means b to a
	{
		if (!getAbsPath(str))
			return "root path has no parent\n";
		pii sourceloc = nameToInode(str);//location of source
		int sourcein = sourceloc.first, sourcep = sourceloc.second;
		if (sourcein != -1)
		{
			if ((opt == "cpy" || opt == "zip") && getMode(sourcein) != FIL)
				return str + NON_FILE;
			if (opt == "dezip"&&getMode(sourcein) != ZIP)
				return str + NON_ZIP;
			if (content == str)
				return "destination and source is the same file\n";
			string source = getData(sourcein, opt == "dezip");//dezip命令需要读取完整字串，不能碰到'\0'停下
			if (opt == "cpy")
				return exec({ "echo", source, content });
			if (opt == "zip")
			{
				if (!source.empty())
					return exec({ "echo", zip(source), content,"z" });
				else return str + IS_EMPTY;
			}
			if (opt == "dezip")
				return exec({ "echo", dezip(source), content });
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
			string chk = allocator.check(names.back(), parent, cnt.first, cnt.second);
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
		if (command.size() >= 3 && command[2] == "-re")
		{
			regex *re = new regex(content);
			ret = find(0, content, re);
			delete re;
		}
		else
			ret = find(0, content, nullptr);
		if (ret.empty())
			ret = "no result found\n";
	}
	if (opt == "append")
	{
		//删文件重写，暴力出奇迹
		string source = getData(in) + str;
		string chk = allocator.check("", -1, 0, ceil(1.0 * source.size() / MAX_SIZE));
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
	if (REQUIRE_WRITE.count(opt))//中途return则不必执行写操作
		exec({ "write" });
	return ret;
}

int FileManager::Allocator::allocateInode(int mode, int p)
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
			self->inodes[i].t = time(nullptr);
			nInode = i;
			break;
		}
	}
	return nInode;
}

int FileManager::Allocator::allocateBlock(int nInode,int size)
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
		//这里保证申请到了一块连续的block，起始点为i，长度为size，而且都已经赋好初值了
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
void FileManager::Allocator::allocateEntry(int parent, int nInode, const string &content)
{
	self->inodes[parent].t = time(nullptr);
	parent = self->inodes[parent].block;
	for (int i = 0; i < 14; ++i)
		if (self->getDBFromIndex(parent).dirs[i].name[0] == '\0')
		{
			::cpy(self->getDBFromIndex(parent).dirs[i].name, content, PATH_SIZE);
			self->getDBFromIndex(parent).dirs[i].inode = nInode;
			break;
		}
}

void FileManager::Allocator::initDirBlock(int nInode, int nBlock, int parent, const string &content, const string &parent_name)
{
	DirBlock &db = self->getDBFromIndex(nBlock);
	for (int i = 0; i < 14; ++i)
		memset(db.dirs[i].name, 0, PATH_SIZE * sizeof(char));
	db.dirs[14].inode = nInode;
	::cpy(db.dirs[14].name, content, PATH_SIZE);
	db.dirs[15].inode = parent;
	::cpy(db.dirs[15].name, parent_name, PATH_SIZE);
}

int FileManager::Allocator::createFile(int parent, const string &name, string source, bool iszip)
{
	int nInode = allocateInode(iszip ? ZIP : FIL, parent);
	int nBlock = allocateBlock(nInode,ceil(1.0*source.size() / MAX_SIZE));
	//本质上zip也是文件,只需要在inode里特别标记一下
	//一次性申请了一个区间内的block,如果source为空，不会影响除了inodes[nInode]之外的地方
	allocateEntry(parent, nInode, name);
	if (!source.empty())
		::cpy(self->getFBFromIndex(nBlock).data, source, MAX_SIZE - 1);//考虑'\0'
	while (source.size() >= MAX_SIZE)
	{
		source = source.substr(MAX_SIZE - 1);
		::cpy(self->getFBFromIndex(++nBlock).data, source, MAX_SIZE - 1);
	}
	return nInode;
}

int FileManager::Allocator::createDir(int parent, const string &name, const string &parentName)
{
	int nInode = allocateInode(DIR, parent);
	int nBlock = allocateBlock(nInode);
	initDirBlock(nInode, nBlock, parent, name, parentName);
	allocateEntry(parent, nInode, name);
	return nInode;
}

string FileManager::Allocator::check(const string &name, int parent, int requiredInodes, int requiredBlocks)
{
	if (name.size() >= PATH_SIZE)
		return TOO_LONG;
	if (parent != -1)//要求检查parent
	{
		bool freeDir = false;
		DirBlock &db = self->getDBFromInode(parent);
		for (int i = 0; i < 14; ++i)
			if (!db.dirs[i].name[0])
				freeDir = true;
		if (!freeDir)
			return FULL_DIR;
	}
	for (int i = 0; i <= MAX_SIZE - requiredBlocks; ++i)
	{
		bool free = true;
		for (int j = 0; j < requiredBlocks; ++j)
			if (self->blockBitmap[i + j])
			{
				free = false;
				break;
			}
		if (free) requiredBlocks = 0;
	}
	for (int i = 0; i <= MAX_SIZE - requiredInodes; ++i)
	{
		bool free = true;
		for (int j = 0; j < requiredBlocks; ++j)
			if (self->inodeBitmap[i + j])
			{
				free = false;
				break;
			}
		if (free) requiredInodes = 0;
	}
	if (requiredBlocks > 0 || requiredInodes > 0)
		return FULL_SYS;
	return "";
}