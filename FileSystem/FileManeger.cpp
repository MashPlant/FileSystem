#include "stdafx.h"
#include "FileManeger.h"

using namespace std;
const string FileManeger::NON_EXIST = " No such file or directory\n";
const string FileManeger::NON_FILE = " is not a file\n";
const string FileManeger::NON_DIR = " is not a directory\n";
const string FileManeger::NON_ZIP = " is not a zip\n";
const string FileManeger::IS_EMPTY = " is empty\n";
const string FileManeger::ALREADY_EXIST = " already exists\n";
const string FileManeger::TOO_LONG = " directory name is too long\n";
const string FileManeger::FULL_DIR = " this dir is full\n";
const string FileManeger::FULL_SYS = " the system doesn't have enoug capacity\n";

const set<string> FileManeger::REQUIRE_DIR = { "cd","rmdir","ls_r" };
const set<string> FileManeger::REQUIRE_FILE = { "cat","rm","append" };
const set<string> FileManeger::REQUIRE_EXIST = { "cd","ls","rmdir","cat","rm","ls_r","append" };
const set<string> FileManeger::REQUIRE_PARENT = { "echo","mkdir","cpy","cpydir","zip" ,"dezip"};
const set<string> FileManeger::REQUIRE_WRITE = { "echo" ,"mkdir" ,"rm" ,"rmdir" ,"cpy" ,"cpydir" ,"append" ,"zip","dezip" };

const map<string, int> FileManeger::ALL_COMMAND =
{ {"time",1},{"exit",1},{"write",1},{"pwd",1},{"cd",2},{"mkdir",2},{"ls",2},{"ls_r",2},{"rmdir",2},{"echo",3},{"cat",2},{"rm",2},{"pwd_r",1},{"cpy",3},{ "zip",3 },{ "dezip",3 },{"cpydir",3},{"find",2},{"find_re",2},{"append",3} };

inline FileManeger::DirBlock& FileManeger::getDBFromInode(int inode)
{
	return blocks[inodes[inode].block].db;
}
inline FileManeger::DirBlock& FileManeger::getDBFromIndex(int index)
{
	return blocks[index].db;
}

inline FileManeger::FileBlock& FileManeger::getFBFromInode(int inode)
{
	return blocks[inodes[inode].block].fb;
}

inline FileManeger::FileBlock& FileManeger::getFBFromIndex(int index)
{
	return blocks[index].fb;
}

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
		ret += getFBFromIndex(inodes[inode].block + i).data;
	return ret;
}

inline string FileManeger::getName(int inode)
{
	if (getMode(inode) == DIR)
		return inode ? getDBFromInode(inode).dirs[14].name : "/";
	if (getMode(inode) == FIL|| getMode(inode) == ZIP)
	{
		DirBlock &db = getDBFromInode(getP(inode));
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
	if (parent != -1)//��Ҫ����parent
	{
		bool freeDir = false;
		DirBlock &db = getDBFromInode(parent);
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
	if (names[0] == ".")//��������մ�������ָ�һ���н��
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
			if (getMode(self) == FIL|| getMode(self) == ZIP)//������Ҫ���ļ���ֻ���������һ������ļ�����ʱiѭ�����˳�
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
			if (i + 1 != names.size())//���������һ��û�ҵ��ǿ��Եģ���������
				return make_pair(-1, -1);
			return make_pair(-1, parent);
		}
	}
	return make_pair(self, parent);
}

void FileManeger::remove(int inode)
{
	if (getMode(inode) == FIL|| getMode(inode) == ZIP)
	{
		Inode &in = inodes[inode];
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			for (int i = 0; i<in.cnt; ++i)
			{
				blockBitmap[inodes[inode].block + i] = false;//��Ǽ��ɣ������������
				blockToInode.erase(inodes[inode].block + i);
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
		if (inode != 0)
		{
			inodeBitmap[inode] = false;
			blockBitmap[inodes[inode].block] = false;
		}
		DirBlock &db = getDBFromInode(inode);
		if (inode != 0)
		{
			inodes[getP(inode)].t = time(nullptr);
			DirBlock &p = getDBFromInode(getP(inode));
			for (int i = 0; i < 14; ++i)
				if (p.dirs[i].inode == inode)
				{
					memset(p.dirs[i].name, 0, sizeof(p.dirs[i].name));
					break;
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

string FileManeger::print(int inode, int shift, bool r)
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
			ret += space + (getMode(db.dirs[i].inode) == DIR ? "/" : "") + child;
			ret += showTime ? string(" last modified:") + ctime(&inodes[db.dirs[i].inode].t) : "\n";
			if (r)
				ret += print(db.dirs[i].inode, shift + 1, r);
		}
	}
	return ret;
}
string FileManeger::find(int inode, const string &match, regex *re)
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
	empty.mode = EMPTY, dir.mode = DIR, dir.t = time(nullptr);
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
					fwrite(f, getDBFromIndex(i).dirs[j].name, getDBFromIndex(i).dirs[j].inode);
			else
				fwrite(f, getFBFromIndex(i).data);
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
			DirBlock &db = getDBFromInode(curr);
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
	if (kind == FIL|| getMode(inode) == ZIP)
	{
		return make_pair(1, inodes[inode].cnt);
	}
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
		if (blockToInode.count(i))//not empty
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

string FileManeger::exec(vector<string> command)
{
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
			swap(content, str);//�������ˣ������ıȽϿ�
	}

	if (!ALL_COMMAND.count(opt))
		return opt + " invalid command\n";
	if (ALL_COMMAND.at(opt) > command.size())//�����Ӳ���
		return "wrong arguments number\n";
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
	else//����->û���� or ��Ҫ�����->Ҫ��parent����->�����������
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
			ret += content + " is a directory " + (showTime ? string(" last modified:") + ctime(&inodes[in].t) : "\n");
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
			ret += getName(in) + "\n";
			ret += print(in, 1, opt == "ls_r");
		}
		else if (getMode(in) == FIL)
		{
			ret += content + " is a file " + (showTime ? string(" last modified:") + ctime(&inodes[in].t) : "\n");
			ret += "the size of " + content + " is " + to_string(inodes[in].sz) + "\n";
			ret += getData(in) + string("\n");
		}
		else if (getMode(in) == ZIP)
		{
			ret += content + " is a zip " +(showTime ? string(" last modified:") + ctime(&inodes[in].t) : "\n");
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
			int nInode = allocator.createFile(parent, name, str, parameter == "z");
			resize(nInode, str.size());
		}
		else
		{
			//�ҿ����inode�������block�������dirs���޸�map
			vector<string> names = split(content, "/");
			string chk = check(names.back(), parent, 1, ceil(1.0*str.size() / MAX_SIZE));
			if (!chk.empty())
				return chk;
			if (str == "\"\"")
				str.clear();
			int nInode = allocator.createFile(parent, names.back(), str, parameter == "z");
			resize(nInode, str.size());
		}
	}
	if (opt == "cat")
	{
		ret = getData(in) + "\n";
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
	//���ǵ�inode��block�����þ���copy����ʱҲ���м��
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
			string source = getData(sourcein);
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
			string chk = check(names.back(), parent, cnt.first, cnt.second);
			if (!chk.empty())
				return chk;
			int nInode = allocator.createDir(parent, names[names.size() - 1], names[names.size() - 2]);
			//��֤���뵽��һƬ������inode��block��������Ҳ�ܱ�֤�������
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
		//ɾ�ļ���д���������漣
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
	if (REQUIRE_WRITE.count(opt))//��;return�򲻱�ִ��д����
		exec({ "write" });
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
			self->inodes[i].t = time(nullptr);
			nInode = i;
			break;
		}
	}
	return nInode;
}

int FileManeger::Allocator::allocateBlock(int nInode,int size)
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
		//���ﱣ֤���뵽��һ��������block����ʼ��Ϊi������Ϊsize�����Ҷ��Ѿ�����ó�ֵ��
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
//Լ�����е�parent����ָparent��inode
void FileManeger::Allocator::allocateEntry(int parent, int nInode, const string &content)
{
	self->inodes[parent].t = time(nullptr);
	parent = self->inodes[parent].block;//��ǰinode��blockһ��һ��Ӧ�ʶ�û�г�����
	for (int i = 0; i < 14; ++i)
		if (self->getDBFromIndex(parent).dirs[i].name[0] == '\0')
		{
			::cpy(self->getDBFromIndex(parent).dirs[i].name, content, PATH_SIZE);
			self->getDBFromIndex(parent).dirs[i].inode = nInode;
			break;
		}
}

void FileManeger::Allocator::initDirBlock(int nInode, int nBlock, int parent, const string &content, const string &parent_name)
{
	DirBlock &db = self->getDBFromIndex(nBlock);
	for (int i = 0; i < 14; ++i)
		memset(db.dirs[i].name, 0, MAX_SIZE * sizeof(char));
	db.dirs[14].inode = nInode;
	::cpy(db.dirs[14].name, content, PATH_SIZE);
	db.dirs[15].inode = parent;
	::cpy(db.dirs[15].name, parent_name, PATH_SIZE);
}

int FileManeger::Allocator::createFile(int parent, const string &name, string source, bool iszip)
{
	int nInode = allocateInode(iszip ? ZIP : FIL, parent);
	int nBlock = allocateBlock(nInode,ceil(1.0*source.size() / MAX_SIZE));
	//������zipҲ���ļ�,ֻ��Ҫ��inode���ر���һ��
	//һ����������һ�������ڵ�block,���sourceΪ�գ�����Ӱ�����inodes[nInode]֮��ĵط�
	allocateEntry(parent, nInode, name);
	if (!source.empty())
		::cpy(self->getFBFromIndex(nBlock).data, source, MAX_SIZE - 1);//����'\0'
	while (source.size() >= MAX_SIZE)
	{
		source = source.substr(MAX_SIZE - 1);
		::cpy(self->getFBFromIndex(++nBlock).data, source, MAX_SIZE - 1);
	}
	return nInode;
}

int FileManeger::Allocator::createDir(int parent, const string &name, const string &parentName)
{
	int nInode = allocateInode(DIR, parent);
	int nBlock = allocateBlock(nInode);
	initDirBlock(nInode, nBlock, parent, name, parentName);
	allocateEntry(parent, nInode, name);
	return nInode;
}