#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <algorithm>
#include <vector>

#include <windows.h>

std::wstring strConvert(const std::string &str, const UINT &encoding = CP_ACP);
std::wstring replace(std::wstring &s, const std::wstring &toReplace, const std::wstring &replaceWith);
void stripQuotes(std::string &str);
std::string prettyFSString(std::ifstream::pos_type nFilesize);
std::string prettyTimeString(int secs);

std::string randomStrGen(int length);

struct tm* convertStrToTime(std::string timeStr, bool onlyDate);
std::string getTime(int f);

int exec(const char* cmd, std::string &res);
DWORD exec(const std::wstring &wsCommand, std::wstring &wsResult, const UINT &nCP);

std::wifstream::pos_type filesize(std::wstring filename);
bool hasPrefix(const std::wstring &s, const std::wstring &prefix);

std::wostream& wendl(std::wostream& out);