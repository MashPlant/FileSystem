#include "stdafx.h"
#include "Util.h"
#include <algorithm>
#include <iostream>
using namespace std;

void writeEmpty(FILE *f, int cnt)
{
	if (cnt == 0)
		return;
	fseek(f, cnt - 1, SEEK_CUR);
	fwrite(f, '\0');
}

char encode(bool *beg, int len)//[)
{
	int d = 0;
	for (int i = 0; i<len; ++i)
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

void cpy(char *dest, const string &str, int maxSize)
{
	maxSize = std::min(maxSize, (int)str.size());
	memcpy(dest, str.c_str(), maxSize * sizeof(char));
	dest[maxSize] = '\0';
}

vector<string> readLine()
{
	vector<string> ret;
	string temp;
	getline(cin, temp);
	return split(temp, " ");
}

vector<string> split(const string &s, const string &pat)//∆”Àÿ µœ÷£¨ŒﬁKMP 
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