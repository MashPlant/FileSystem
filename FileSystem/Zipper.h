#ifndef ZIPPER_H
#define ZIPPER_H
#include <queue>
#include <string>
#include <map>
//保证DeZipper(Zipper(s)) == s
class Zipper//std::string->01序列->文件,必须生成直接放进文件里的东西
{
private:
	const static int ALPHABET_SIZE = 128;//char类型只有0-127有意义
	std::map<std::string, char> codeToChar;
	std::map<char, std::string> charToCode;

	struct node
	{
		int cnt = 0;
		int self = -1;//在数组中自己的位置 
		int left = -1;
		int right = -1;
		char data = -1;
		node(int _self = -1, int _cnt = 0, int _left = -1, int _right = -1) :self(_self), cnt(_cnt), left(_left), right(_right) {	}
		bool operator<(const node& right) const
		{
			return cnt > right.cnt;
		}
	};
	node tree[1000];
	int cnt[1000] = { 0 };
	void initAlphabet(const std::string &input)
	{
		for (int i = 0; i<input.size(); ++i)
			++cnt[input[i]];
		std::priority_queue<node> pq;
		int pos = 0;
		for (int i = 0; i<ALPHABET_SIZE; ++i)
		{
			if (cnt[i])
			{
				tree[pos++] = node(pos, cnt[i]);
				tree[pos - 1].data = i;
				pq.push(tree[pos - 1]);
			}
		}
		int root = -1;
		while (!pq.empty())
		{
			node a = pq.top(), b;
			pq.pop();
			if (pq.empty())
			{
				root = a.self;
				break;
			}
			b = pq.top();
			pq.pop();
			node p(pos++, a.cnt + b.cnt, a.self, b.self);//非叶节点没有data 
			tree[pos - 1] = p;
			pq.push(p);
		}
		if (tree[root].left == -1 || tree[root].right == -1)//根本身也是叶子
			buildMap(root, "0");
		else buildMap(root, "");
	}
	void buildMap(int r, std::string pre)
	{
		bool leaf = true;
		if (tree[r].left != -1)
			buildMap(tree[r].left, pre + '0'), leaf = false;
		if (tree[r].right != -1)
			buildMap(tree[r].right, pre + '1'), leaf = false;
		if (leaf)
		{
			charToCode[tree[r].data] = pre;
			codeToChar[pre] = tree[r].data;
		}
	}

	char decode(const std::string &s)//"01010"->char
	{
		//对应下面decode的时候，也只会生成对应char
		//总之decode和decode互为反函数
		int d = 0;
		int len = s.size();
		for (int i = 0; i<len; ++i)
		{
			d |= (s[i] == '1') << (len - 1 - i);
		}
		return d;
	}
public:
	std::string operator()(const std::string &input)
	{
		initAlphabet(input);
		std::string zipped;//压缩后的input，不能直接放进文件
		for (int i = 0; i<input.size(); ++i)
		{
			zipped += charToCode[input[i]];
		}
		std::string ret;//直接放进文件
		ret += codeToChar.size();
		for (auto c : codeToChar)
		{
			ret += c.first.size();
			ret += decode(c.first);
			ret += c.second;
		}
		//存放input的size，避免因为最后一位尾部补0而识别出多余的字符
		int countLetters = input.size();
		for (int i = 0; i<4; ++i)
		{
			ret += countLetters&((1 << 8) - 1);
			countLetters >>= 8;
		}

		for (int i = 0; i<zipped.size(); ++i)
		{
			if (i + 7 < zipped.size())
			{
				ret += decode(zipped.substr(i, 8));
				i += 7;
			}
			else
			{
				std::string sub = zipped.substr(i);
				for (; sub.size() != 8;)//低位补0
					sub += '0';
				ret += decode(sub);
				break;
			}
		}
		codeToChar.clear();
		charToCode.clear();
		return ret;
	}
	Zipper() {}
};

class DeZipper//文件->01序列->std::string
{
private:
	const static int ALPHABET_SIZE = 128;
	std::map<std::string, char> codeToChar;

	std::string encode(char c, int bitnum)
	{
		std::string ret;
		for (int i = bitnum - 1; i >= 0; --i)
		{
			ret += c&(1 << i) ? "1" : "0";
		}
		return ret;
	}

public:
	std::string operator()(const std::string &bitstream)
	{
		int pos = 0;
		int alphabetSize = bitstream[pos++];
		for (int i = 0; i < alphabetSize; ++i)
		{
			int bitnum = bitstream[pos++];
			char code = bitstream[pos++];
			char letter =bitstream[pos++];
			codeToChar[encode(code, bitnum)] = letter;
		}
		int countLetters = *reinterpret_cast<const int*>(&bitstream[pos]);
		pos += 4;
		std::string coded;
		for (; pos<bitstream.size(); ++pos)
			coded += encode(bitstream[pos], 8);
		std::string ret;//原string
		for (int i = 0; i < coded.size() && countLetters; ++i)
		{
			for (int j = 1; j + i <= coded.size(); ++j)
			{
				std::string sub = coded.substr(i, j);
				if (codeToChar.count(sub))
				{
					ret += codeToChar.at(sub);
					--countLetters;
					i += j - 1;
					break;
				}
			}
		}
		codeToChar.clear();
		return ret;
	}
	DeZipper() {}
};

#endif
