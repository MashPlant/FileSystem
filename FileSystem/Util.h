#pragma once
#include <string>
#include <vector>
#include <cstdlib>
std::vector<std::string> split(const std::string &s, const std::string &pat)//∆”Àÿ µœ÷£¨ŒﬁKMP 
{
	int szs = s.size(), szp = pat.size();
	if (szs<szp)
		return std::vector<std::string>();
	std::vector<int> match;
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
	std::vector<std::string> ret;
	int curr = 0;
	for (int i = 0; i<szm; ++i)
	{
		ret.push_back(std::string(s.begin() + curr, s.begin() + match[i]));
		curr = match[i] + szp;
	}
	return ret;
}

template<typename T>
void fwrite(FILE *f,const T &t)
{
	fwrite(static_cast<void const*>(&t), sizeof(T), 1, f);
}

template<typename T, typename... Args>
void fwrite(FILE *f, const T &t, const Args&... args)
{
	fwrite(static_cast<const void*>(&t), sizeof(T), 1, f);
	fwrite(f, args...);
}

template<typename T>
void fread(FILE *f, T &t)
{
	fread(static_cast<void*>(&t), sizeof(T), 1, f);
}

template<typename T,typename... Args>
void fread(FILE *f,T &t,Args&... args)
{
	fread(static_cast<void*>(&t), sizeof(T), 1, f);
	fread(f, args...);
}

void writeEmpty(FILE *f, int cnt)
{
	if (cnt == 0)
		return;
	fseek(f, cnt - 1, SEEK_CUR);
	fwrite(f, '\0');
}

template<size_t N>
void fread(FILE *f,char (&s)[N])
{
	char c;
	for (int i = 0; i < N; ++i)
	{
		fread(f, c);
		if (c)
			s[i] = c;
		else
		{
			fseek(f, N - i, SEEK_CUR);
			memset(s + i, 0, (N - i) * sizeof(char));
			break;
		}
	}
}

template<size_t N>
void fwrite(FILE *f, const char(&s)[N])
{
	for (int i=0;i<N;++i)
	{
		if (s[i])
			fwrite(f, s[i]);
		else
		{
			writeEmpty(f, N - 1);
			break;
		}
	}
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