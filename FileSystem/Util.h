#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <iostream>

template<typename T>
void fwrite(FILE *f, const T &t)
{
	fwrite(static_cast<void const*>(&t), sizeof(T), 1, f);
}

template<typename T, typename... Args>
void fwrite(FILE *f, const T &t, const Args&... args)
{
	fwrite(f, t);
	fwrite(f, args...);
}

inline std::vector<std::string> split(const std::string &s, const std::string &pat) 
{
	std::vector<std::string> ret;
	int p = 0, oldp = 0;
	while ((p = s.find(pat, p)) != std::string::npos)
		ret.push_back(s.substr(oldp, p - oldp)), ++p, oldp = p;
	ret.push_back(s.substr(oldp));
	return ret;
}

inline std::vector<std::string> readLine()
{
	std::string temp;
	getline(std::cin, temp);
	auto it = std::unique(temp.begin(), temp.end(), [](char a, char b) { return a == ' '&&b == ' '; });
	temp.erase(it, temp.end());
	return split(temp, " ");
}

inline void writeEmpty(FILE *f, int cnt)
{
	if (cnt == 0)
		return;
	fseek(f, cnt - 1, SEEK_CUR);
	fwrite(f, '\0');
}

inline void cpy(char *dest, const std::string &str, int maxSize)
{
	maxSize = std::min(maxSize, (int)str.size());
	memcpy(dest, str.c_str(), maxSize * sizeof(char));
	dest[maxSize] = '\0';
}
template<size_t N>
std::string fullCharArrToStr(const char (&s)[N])
{
	std::string ret;
	for (int i = 0; i < N; ++i)
		ret += s[i];
	return ret;
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
}

inline FILE* open(const char *path,const char *mode)
{
	FILE *f = nullptr;
	while (!f)
		f = fopen(path, mode);
	return f;
}
#endif
