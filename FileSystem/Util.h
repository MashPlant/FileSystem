#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <vector>
#include <cstdlib>

std::vector<std::string> split(const std::string &s, const std::string &pat);
std::vector<std::string> readLine();
char encode(bool *beg, int len);//[)
void decode(char c, bool *beg, int len);//[)
void writeEmpty(FILE *f, int cnt);
void cpy(char *dest, const std::string &str, int maxSize);

template<typename T>
void fwrite(FILE *f,const T &t)
{
	fwrite(static_cast<void const*>(&t), sizeof(T), 1, f);
}

template<typename T, typename... Args>
void fwrite(FILE *f, const T &t, const Args&... args)
{
	fwrite(f, t);
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
	fread(f, t);
	fread(f, args...);
}

template<size_t N>
void fread(FILE *f,char (&s)[N])
{
	fread(&s, sizeof(char), N, f);
	/*char c;
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
	}*/
}

template<size_t N>
void fwrite(FILE *f, const char(&s)[N])
{
	for (int i = 0; i < N; ++i)
	{
		if (s[i])
			fwrite(f, s[i]);
		else
		{
			writeEmpty(f, N - i);
			break;
		}
	}
}

#endif
