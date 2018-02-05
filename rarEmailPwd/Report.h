#pragma once
#include <forward_list>
#include <string>
#include <time.h>

struct ReportItem {
	int bResult;
	std::wstring sFolderPath;
	std::wstring sMode;
	std::wstring sTimeStart;
	std::wstring sTimeFinish;
	std::wstring sDuration;
	std::wstring sFilesize;
};

class Report
{
	std::forward_list<ReportItem> items;
	int nCnt;
public:
	Report();
	~Report();
	void addItem(ReportItem &new_item);
	void createReport(const std::wstring &sFilename, const std::wstring sTimeReport);
};
