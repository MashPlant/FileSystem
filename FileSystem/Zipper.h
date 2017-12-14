#ifndef ZIPPER_H
#define ZIPPER_H
#include <queue>
#include <string>
#include <map>
//��֤DeZipper(Zipper(s)) == s
class Zipper//std::string->01����->�ļ�,��������ֱ�ӷŽ��ļ���Ķ���
{
private:
	const static int ALPHABET_SIZE = 128;//char����ֻ��0-127������
	std::map<std::string, char> codeToChar;
	std::map<char, std::string> charToCode;

	struct node
	{
		int cnt = 0;
		int self = -1;//���������Լ���λ�� 
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
			node p(pos++, a.cnt + b.cnt, a.self, b.self);//��Ҷ�ڵ�û��data 
			tree[pos - 1] = p;
			pq.push(p);
		}
		if (tree[root].left == -1 || tree[root].right == -1)//������Ҳ��Ҷ��
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
	char label(char c)
	{
		return c | (1 << 7);
	}
	char decode(const std::string &s)//"01010"->char
	{
		//��Ӧ����decode��ʱ��Ҳֻ�����ɶ�Ӧchar
		//��֮decode��decode��Ϊ������
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
		std::string zipped;//ѹ�����input������ֱ�ӷŽ��ļ�
		for (int i = 0; i<input.size(); ++i)
		{
			zipped += charToCode[input[i]];
		}
		std::string ret;//ֱ�ӷŽ��ļ�
		ret += label(codeToChar.size());
		for (auto c : codeToChar)
		{
			ret += label(c.first.size());
			ret += label(decode(c.first));
			ret += label(c.second);
		}
		//���input��size��������Ϊ���һλβ����0��ʶ���������ַ�
		//����ֻ��4��char��28λ����Ÿ�int(��λ��ǰ)����Ϊ�ļ�ϵͳʵ����������
		int countLetters = input.size();
		for (int i = 0; i<4; ++i)
		{
			ret += label(countLetters&((1 << 7) - 1));
			countLetters >>= 7;
		}

		for (int i = 0; i<zipped.size(); ++i)
		{
			if (i + 6 < zipped.size())
			{
				ret += label(decode(zipped.substr(i, 7)));
				i += 6;
			}
			else
			{
				std::string sub = zipped.substr(i);
				for (; sub.size() != 7;)//��λ��0����һ�����⣺������󲹵�0�ֱ�����һ�����ַ��������Ȳ�����
					sub += '0';
				ret += label(decode(sub));
				break;
			}
		}
		codeToChar.clear();
		charToCode.clear();
		return ret;
	}
	Zipper() {}
};

class DeZipper//�ļ�->01����->std::string
{
private:
	const static int ALPHABET_SIZE = 128;//char����ֻ��0-127������
	std::map<std::string, char> codeToChar;

	char delable(char c)
	{
		return c & ~(1 << 7);
	}

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
		int alphabetSize = delable(bitstream[pos++]);
		for (int i = 0; i < alphabetSize; ++i)
		{
			int bitnum = delable(bitstream[pos++]);
			char code = delable(bitstream[pos++]);
			char letter = delable(bitstream[pos++]);
			codeToChar[encode(code, bitnum)] = letter;
		}
		int countLetters = 0;
		for (int i = 0; i<4; ++i)
		{
			countLetters += delable(bitstream[pos++]) << (i * 7);
		}
		std::string coded;
		for (; pos<bitstream.size(); ++pos)
		{
			coded += encode(delable(bitstream[pos]), 7);
		}
		std::string ret;//ԭstd::string
		for (int i = 0; i < coded.size() && countLetters; ++i)
		{
			for (int j = 1; j + i <= coded.size(); ++j)
			{
				std::string sub = coded.substr(i, j);
				if (codeToChar.count(sub))
				{
					ret += char(codeToChar.at(sub));
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
