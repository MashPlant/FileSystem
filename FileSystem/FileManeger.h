#pragma once
#include <string>
#include <iostream>
#include <cstdio>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
#include <regex>
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
		void read(FILE *f)
		{
			fscanf(f, " %d%d%d%d ", &id, &mode, &sz, &block);
		}
		void print(FILE *f)
		{
			
			fprintf(f, "%d %d %d %d ", id, mode, sz, block);
		}
	};
	struct FileBlock
	{
		char data[MAX_SIZE] = { '\0' };
		int p;//ָ����inode
	};

	struct DirEntry
	{
		char name[PATH_SIZE] = {'\0'};
		int inode = 0;
	};

	//"�Լ���֪���Լ�������"

	struct DirBlock
	{
		DirEntry dirs[16];
		int p;
		//14��ʾ��ǰĿ¼��15��ʾ�ϼ�Ŀ¼��������ʹ��
		//����ֻ�������ֺ�������Ϣ���൱��ֻ��������ļ�/�ļ���
		//�ļ�/�ļ��е�ʵ����Ҫ��inode��Inode[]���ҵ���Ӧ��������ͨ�������ҵ�block
	};

	FileBlock fbs[MAX_SIZE];
	DirBlock dbs[MAX_SIZE];
	//a little waste of memory

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
	vector<string> split(const string &s, const string &pat)//����ʵ�֣���KMP 
	{
		int szs = s.size(), szp = pat.size();
		if (szs<szp)
			return vector<string>();
		vector<int> match;
		for (int i = 0; i <= szs - szp; ++i)
		{
			bool b = true;
			for (int j = 0; j<szp; ++j)
			{
				if (s[i + j] != pat[j])
				{
					b = false;
					break;
				}
			}
			if (b)
				match.push_back(i);
		}
		match.push_back(szs);
		int szm = match.size();
		vector<string> ret;
		int curr = 0;
		for (int i = 0; i<szm; ++i)
		{
			ret.push_back(string(s.begin() + curr, s.begin() + match[i]));
			curr = match[i] + szp;
		}
		return ret;
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
		for (int i = 1; i < names.size(); ++i)//names[0]ֻ����"." ".." ""֮һ,������
		{
			bool found = false;
			for (int j = 0; j < 14; ++j)
			{
				if (getKind(self) == FIL)//������Ҫ���ļ���ֻ���������һ������ļ�����ʱiѭ�����˳�
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
				if (i + 1 != names.size())//���������һ��û�ҵ��ǿ��Եģ���������
					return make_pair(-1, -1);
				return make_pair(-1, parent);
			}
		}
		return make_pair(self, parent);
	}
	//todo �ļ�����inodeҲҪ��ӳ��

	map<int, int> blockToInode;

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
	bool cpy(char *dest, const string &str,int maxSize)
	{
		if (str.size() >= maxSize)//����'\0',�������
		{
			return false;
		}
		memcpy(dest, str.c_str(), str.size() * sizeof(char));
		dest[str.size()] = '\0';
		return true;
	}
	void cpy(int dest,int sour)//��Ȼ���ܸ���parent������
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
	void writeEmpty(FILE *f,int cnt)
	{
		if (cnt == 0)
			return;
		fseek(f, cnt - 1, SEEK_CUR);
		fputc(0, f);
	}
	void init()
	{
		FILE *f = fopen(path.c_str(), "w");
		//superblock
		for (int j = 0; j < 2; ++j)
		{
			fputc(1<<7, f);
			writeEmpty(f, (MAX_SIZE>>3) - 1);
		}
		//fseek(f, 7 * 1024, SEEK_CUR);
		int a = ftell(f);
		int b = a;
		//inode
		Inode empty, dir;
		empty.mode = EMPTY, dir.mode = DIR;
		dir.print(f);
		for (int i = 1; i < MAX_SIZE; ++i)
			empty.print(f);
		a = ftell(f);
		b = a;
		//block
		writeEmpty(f, MAX_SIZE*MAX_SIZE);
		a = ftell(f);
		b = a;
		fclose(f);
	}
	void write(FILE *f)
	{
		//superblock
		for (int i = 0; i < MAX_SIZE; i += 8)
		{
			char c = encode(inodeBitmap + i, 8);
			fputc(c, f);
		}
		for (int i = 0; i < MAX_SIZE; i += 8)
		{
			char c = encode(blockBitmap + i, 8);
			fputc(c, f);
		}
		//inode
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			Inode &in = inodes[i];
			in.print(f);
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
						for (int k = 0; k < PATH_SIZE; ++k)
						{
							if (!db.dirs[j].name[k])
							{
								writeEmpty(f, PATH_SIZE - k);
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
							writeEmpty(f, MAX_SIZE - j);
							break;
						}
						fputc(fb.data[j], f);
					}
				}
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
		FILE* f = fopen(_path.c_str(), "r");
		if (!f)
		{
			init();
			f = fopen(_path.c_str(), "r");
		}
		for (int i = 0; i < MAX_SIZE;i += 8)
		{
			char c = fgetc(f);
			decode(c, inodeBitmap + i, 8);
		}
		for (int i = 0; i < MAX_SIZE; i += 8)
		{
			char c = fgetc(f);
			decode(c, blockBitmap + i, 8);
		}
		for (int i = 0; i < MAX_SIZE; ++i)
		{
			inodes[i].read(f);
			if (inodes[i].mode != EMPTY)
			{
				blockToInode[inodes[i].block] = inodes[i].id;
			}
		}
		fscanf(f," ");//����ո�
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
						for (int k = 0; k < PATH_SIZE; ++k)
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
		FILE *f = fopen(path.c_str(), "w");
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
		else//����->û���� or ��Ҫ�����->Ҫ��parent����->�����������
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
				int old_sz=inodes[in].sz;
				if (cpy(fbs[inodes[in].block].data, str, MAX_SIZE))
				{
					return str + TOO_LONG;
				}
				resize(in, (int)str.size() - old_sz);
			}
			else
			{
				//�Ĳ�:�ҿ����inode�������block�������dirs���޸�map
				vector<string> names = split(content, "/");
				if (str.size() >= MAX_SIZE)
				{
					return str + TOO_LONG;
				}
				if (names.back().size() >= PATH_SIZE)
				{
					return content + TOO_LONG;
				}
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
			FILE *f = fopen(path.c_str(), "w");
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
const string FileManeger::TOO_LONG = " is too long\n";

const set<string> FileManeger::REQUIRE_DIR = {"cd","rmdir","ls_r"};
const set<string> FileManeger::REQUIRE_FILE = {"cat","rm"};
const set<string> FileManeger::REQUIRE_EXIST = { "cd","ls","rmdir","cat","rm","ls_r"};
const set<string> FileManeger::REQUIRE_PARENT = {"echo","mkdir","cpy","cpydir"};
const set<string> FileManeger::ALL_COMMAND = { "pwd","cd","mkdir","ls","ls_r","rmdir","echo","cat","rm","pwd_r","cpy","cpydir","find","find_re"};

