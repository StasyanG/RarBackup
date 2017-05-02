#include "utility.h"

std::wstring strConvert(const std::string &str, const UINT &encoding) {
	int size_needed = MultiByteToWideChar(encoding, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(encoding, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}//str2unicode

std::wstring replace(std::wstring &s, const std::wstring &toReplace, const std::wstring &replaceWith)
{
	std::string::size_type pos = 0u;
	while ((pos = s.find(toReplace, pos)) != std::string::npos) {
		s.replace(pos, toReplace.length(), replaceWith);
		pos += replaceWith.length();
	}
	return s;
}//replace

void stripQuotes(std::string &str) {
	if (str[0] == '\"')
		str = str.substr(1);
	if (str[str.length() - 1] == '\"')
		str = str.substr(0, str.length() - 1);
}//stripQuotes

std::string prettyFSString(std::ifstream::pos_type nFilesize) {
	std::string sFilesize;
	double nFS = (double)nFilesize / 1024. / 1024. / 1024.;
	if (nFS >= 1.0) {
		char buff[100];
		snprintf(buff, sizeof(buff), "%.3f", nFS);
		sFilesize = buff;
		sFilesize += " GB";
	}
	else {
		nFS = (double)nFilesize / 1024. / 1024.;
		if (nFS >= 1.0) {
			char buff[100];
			snprintf(buff, sizeof(buff), "%.3f", nFS);
			sFilesize = buff;
			sFilesize += " MB";
		}
		else {
			nFS = (double)nFilesize / 1024.;
			if (nFS >= 1.0) {
				char buff[100];
				snprintf(buff, sizeof(buff), "%.3f", nFS);
				sFilesize = buff;
				sFilesize += " Kb";
			}
			else {
				sFilesize = std::to_string(nFS) + " bytes";
			}
		}
	}
	return sFilesize;
}

std::string prettyTimeString(int secs) {
	int nDays = (int)(secs / 3600 / 24);
	int nHours = (int)((secs - nDays * 3600 * 24) / 3600);
	int nMinutes = (int)((secs - nDays * 3600 * 24 - nHours * 3600) / 60);
	int nSeconds = (int)((secs - nDays * 3600 * 24 - nHours * 3600 - nMinutes * 60));
	std::string sDuration = std::to_string(nDays) + "d" + std::to_string(nHours) + "h" + std::to_string(nMinutes) + "m" + std::to_string(nSeconds) + "s";
	return sDuration;
}

std::string randomStrGen(int length) {
	static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
	std::string result;
	result.resize(length);

	for (int i = 0; i < length; i++)
		result[i] = charset[rand() % charset.length()];

	return result;
}//randomStrGen


struct tm* convertStrToTime(std::string timeStr, bool onlyDate = false) {
	time_t t = time(0);
	struct tm* date = localtime(&t);

	size_t pos1 = 0;
	size_t pos2 = timeStr.find("-");
	date->tm_year = atoi(timeStr.substr(0, 4).c_str()) - 1900;
	pos1 = pos2 + 1;
	pos2 = timeStr.find("-", pos1);
	date->tm_mon = atoi(timeStr.substr(pos1, 2).c_str()) - 1;
	pos1 = pos2 + 1;
	pos2 = timeStr.find(" ", pos1);
	date->tm_mday = atoi(timeStr.substr(pos1, 2).c_str());
	if (!onlyDate) {
		pos1 = pos2 + 1;
		pos2 = timeStr.find(":", pos1);
		date->tm_hour = atoi(timeStr.substr(pos1, 2).c_str());
		pos1 = pos2 + 1;
		pos2 = timeStr.find(":", pos1);
		date->tm_min = atoi(timeStr.substr(pos1, 2).c_str());
		pos1 = pos2 + 1;
		date->tm_sec = atoi(timeStr.substr(pos1, timeStr.length() - pos1).c_str());
	}
	return date;
}//convertStrToTime

std::string getTime(int f)
{
	std::string cur_time;
	time_t t = time(0);
	struct tm * now = localtime(&t);
	char buf[25];
	if (!f)
	{
		// year
		_itoa(now->tm_year + 1900, buf, 10);
		cur_time.append(buf);
		cur_time.append("-");

		// month
		_itoa(now->tm_mon + 1, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
		cur_time.append("-");

		// day
		_itoa(now->tm_mday, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
		cur_time.append(" ");

		// hours
		_itoa(now->tm_hour, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
		cur_time.append(":");

		// minutes
		_itoa(now->tm_min, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
		cur_time.append(":");

		// seconds
		_itoa(now->tm_sec, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
	}
	else if (f == 1)
	{
		_itoa(now->tm_year + 1900, buf, 10);
		cur_time.append(buf);
		cur_time.append("_");
		_itoa(now->tm_mon + 1, buf, 10);
		cur_time.append(buf);
		cur_time.append("_");
		_itoa(now->tm_mday, buf, 10);
		cur_time.append(buf);
		cur_time.append("_");
		_itoa(now->tm_hour, buf, 10);
		cur_time.append(buf);
		_itoa(now->tm_min, buf, 10);
		cur_time.append(buf);
		_itoa(now->tm_sec, buf, 10);
		cur_time.append(buf);
	}
	else if (f == 2) { // only date in format YYYY-MM-DD
		_itoa(now->tm_year + 1900, buf, 10);
		cur_time.append(buf);
		cur_time.append("-");
		_itoa(now->tm_mon + 1, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
		cur_time.append("-");
		_itoa(now->tm_mday, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
	}
	else if (f == 3) { // only date in format YYYY_MM_DD
		_itoa(now->tm_year + 1900, buf, 10);
		cur_time.append(buf);
		cur_time.append("_");
		_itoa(now->tm_mon + 1, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
		cur_time.append("_");
		_itoa(now->tm_mday, buf, 10);
		if (strlen(buf) == 1)
			cur_time.append("0" + std::string(buf));
		else
			cur_time.append(buf);
	}

	return cur_time;
}//getTime


int exec(const char* cmd, std::string &result) {
	char buffer[128];
	result = "";

	std::string command = cmd;
	command += " 2>&1";

	FILE* pipe = _popen(command.c_str(), "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
	}
	catch (...) {
		_pclose(pipe);
		throw;
	}
	return _pclose(pipe);
}//exec

DWORD exec(const std::wstring &wsCommand, std::wstring &wsResult, const UINT &nCP) {

	DWORD exit_code;

	// we create temporary file to forward cmd output to it
	std::wstring wsTmpFilename = L"tmp";

	STARTUPINFOW      SI = { 0 };
	PROCESS_INFORMATION PI = { 0 };
	SI.cb = sizeof(STARTUPINFO);

	// extend command to forward output to temporary file
	std::wstring wsExtendedCommand = L"cmd.exe /C \"" + wsCommand + L" > " + wsTmpFilename + L" 2>&1\"";

	// start process
	std::vector<WCHAR> V(wsExtendedCommand.length() + 1);
	for (int i = 0; i< (int)wsExtendedCommand.length(); i++)
		V[i] = wsExtendedCommand[i];
	CreateProcessW(NULL, &V[0], 0, 0, FALSE, 0, 0, 0, &SI, &PI);
	WaitForSingleObject(PI.hProcess, INFINITE);
	GetExitCodeProcess(PI.hProcess, &exit_code);

	CloseHandle(PI.hProcess);
	CloseHandle(PI.hThread);

	// get output from temporary file
	std::ifstream fFile;
	std::string res = "";
	fFile.open(std::string(wsTmpFilename.begin(), wsTmpFilename.end()));
	if (fFile.is_open())
	{
		std::string line;
		while (getline(fFile, line)) {
			res += line + "\r\n";
		}
		fFile.close();
	}

	wsResult = strConvert(res, nCP);

	// remove temporary file
	system(("del " + std::string(wsTmpFilename.begin(), wsTmpFilename.end())).c_str());

	return exit_code;
}


std::wifstream::pos_type filesize(std::wstring filename)
{
    std::wifstream in(filename, std::wifstream::ate | std::wifstream::binary);
	return in.tellg();
}

bool hasPrefix(const std::wstring &s, const std::wstring &prefix) {
	return (s.size() >= prefix.size()) && s.find(prefix) == 0;
}


std::wostream& wendl(std::wostream& out)
{
	out.put(L'\r');
	out.put(L'\n');
	out.flush();
	return out;
}//wendl