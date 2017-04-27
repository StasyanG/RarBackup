#include "Logger.h"

#include "null_wcodecvt.h"

null_wcodecvt wcodec(1);
std::locale wloc(std::locale::classic(), &wcodec);

Logger::Logger(std::wstring folder, std::wstring filenameInf, std::wstring filinameErr, bool verbose, const UINT &encoding = CP_ACP) {
	m_bInitialized = true;

	std::wstring s = L"if not exist \"" + folder + L"\" mkdir " + folder;
	system(std::string(s.begin(), s.end()).c_str());

	this->m_fFileInf.imbue(wloc);
	this->m_fFileInf.open(folder + L"\\" + filenameInf, std::ios::out | std::ios::binary | std::ios::app);
	if (this->m_fFileInf.is_open() == false) {
		std::wcout << L"Failed to open " + folder + filenameInf << std::endl;
		m_bInitialized = false;
	}
	this->m_fFileErr.imbue(wloc);
	this->m_fFileErr.open(folder + L"\\" + filinameErr, std::ios::out | std::ios::binary | std::ios::app);
	if (this->m_fFileErr.is_open() == false) {
		std::wcout << L"Failed to open " + folder + filinameErr << std::endl;
		m_bInitialized = false;
	}
	this->m_bVerbose = verbose;
	this->m_uiCP = encoding;
}

Logger::~Logger() {
}

const bool Logger::isInitialized() {
	return m_bInitialized;
}

void Logger::d(std::wstring TAG, std::wstring msg) {
	std::wstring to_print = L"[" + strConvert(getTime(0), m_uiCP) + L"] INF | " + TAG + L" | " + msg;
	this->m_fFileInf << /*UTF_BOM <<*/ to_print << wendl;
	if (m_bVerbose) {
		std::wcout << to_print << std::endl;
	}
}

void Logger::e(std::wstring TAG, std::wstring msg) {
	std::wstring to_print = L"[" + strConvert(getTime(0), m_uiCP) + L"] ERR | " + TAG + L" | " + msg;
	this->m_fFileErr << /*UTF_BOM <<*/ to_print << wendl;
	if (m_bVerbose) {
		std::wcout << to_print << std::endl;
	}
}