#include "null_wcodecvt.h"
#include "Report.h"
#include "utility.h"
#include <algorithm>

bool compRes(const ReportItem & a, const ReportItem & b);
bool compTime(const ReportItem & a, const ReportItem & b);

Report::Report() {
	nCnt = 0;
}

Report::~Report() {
}

void Report::addItem(ReportItem &new_item) {
	items.push_front(new_item);
	nCnt++;
}

void Report::createReport(const std::wstring &sFilename, const std::wstring sTimeReport) {
	null_wcodecvt wcodec(1);
	std::locale wloc(std::locale::classic(), &wcodec);

	std::wofstream fReport;
	fReport.imbue(wloc);
	fReport.open(
		sFilename,
		std::ios::out | std::ios::binary | std::ios::app
	);
	if (fReport.is_open())
	{
		fReport << UTF_BOM << wendl;

		fReport << L"<html>" << wendl
			<< L"<head>" << wendl
			<< L"<title>RarBackup Report " << sTimeReport << "</title>" << wendl
			<< L"</head>" << wendl
			<< L"<body>" << wendl
			<< L"<h2>RarBackup Report " << sTimeReport << "</h2>" << wendl;

		// Sort by Result
		this->items.sort(compRes);

		// Count successes and errors
		int nSuccessCnt = 0, nErrorCnt = 0;
		for (ReportItem item : this->items) {
			if (item.bResult != 0)
				continue;
			nErrorCnt++;
		}
		nSuccessCnt = nCnt - nErrorCnt;

		fReport << L"<h3>"
			<< L"<span style = \"color: #00cc00;\"><b>Success:</b> " << std::to_wstring(nSuccessCnt) << "</span>"
			<< L" | <span style = \"color: #cc0000;\"><b>With errors:</b> " << std::to_wstring(nErrorCnt) << "</span>"
			<< L"</h3>" << wendl;

		fReport << L"<h3>Table of results</h3>" << wendl
			<< L"<table border=\"1\" cellpadding=\"5\">" << wendl
			<< L"<thead>" << wendl
			<< L"<th>Result</th>" << wendl
			<< L"<th>Folder</th>" << wendl
			<< L"<th>Backup Mode</th>" << wendl
			<< L"<th>Time (start)</th>" << wendl
			<< L"<th>Time (finish)</th>" << wendl
			<< L"<th>Duration</th>" << wendl
			<< L"<th>Filesize</th>" << wendl
			<< L"</thead>" << wendl;

		fReport << L"<tbody>" << wendl;

		for (ReportItem item : this->items) {
			fReport << L"<tr>" << wendl
				<< L"<td><span style = \"color: "
				<< (item.bResult != 0 ? L"#00cc00;\">Success" : L"#cc0000;\">With errors")
				<< L"</span></td>" << wendl
				<< L"<td>" << item.sFolderPath << L"</td>" << wendl
				<< L"<td>" << item.sMode << L"</td>" << wendl
				<< L"<td>" << item.sTimeStart << L"</td>" << wendl
				<< L"<td>" << item.sTimeFinish << L"</td>" << wendl
				<< L"<td>" << item.sDuration << L"</td>" << wendl
				<< L"<td>" << item.sFilesize << L"</td>" << wendl
				<< L"</tr>" << wendl;
		}

		fReport << L"</tbody>" << wendl
			<< L"</table>" << wendl
			<< L"</body>" << wendl
			<< L"</html>" << wendl;

		fReport.close();
	}
}

bool compRes(const ReportItem & a, const ReportItem & b) {
	return a.bResult < b.bResult;
}
bool compTime(const ReportItem & a, const ReportItem & b) {
	return std::lexicographical_compare(
		a.sTimeStart.begin(),
		a.sTimeStart.end(),
		b.sTimeFinish.begin(),
		b.sTimeFinish.end()
	);
}