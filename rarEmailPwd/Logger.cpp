#include "Logger.h"
#include "null_wcodecvt.h"

null_wcodecvt wcodec(1);
std::locale wloc(std::locale::classic(), &wcodec);

Logger::Logger() {
	this->setDefaultSettings();
}

Logger::~Logger() {
}

void Logger::setDefaultSettings() {
	this->m_bInitialized = false;
	this->m_sFolder = L".\\logs";
	this->m_sFilenameInf = L"rarEmailPwd_log.log";
	this->m_sFilenameErr = L"rarEmailPwd_err.log";
	bool m_bVerbose = false;
	UINT m_dEncoding = CP_ACP;
}
void Logger::setFolder(std::wstring folder) {
	this->m_sFolder = folder;
}
void Logger::setFilenameInf(std::wstring filenameInf) {
	this->m_sFilenameInf = filenameInf;
}
void Logger::setFilenameErr(std::wstring filinameErr) {
	this->m_sFilenameErr = filinameErr;
}
void Logger::setVerbose(bool verbose) {
	this->m_bVerbose = verbose;
}
void Logger::setEncoding(const UINT &encoding) {
	this->m_uiCP = encoding;
}

int Logger::init() {
	// 1 - Successfully initialized, 0 - Errors
	this->m_bInitialized = false;
	bool errors = false;

	std::wstring s = L"if not exist \"" + this->m_sFolder + L"\" mkdir " + this->m_sFolder;
	system(std::string(s.begin(), s.end()).c_str());
	this->m_fFileInf.imbue(wloc);
	this->m_fFileInf.open(this->m_sFolder + L"\\" + this->m_sFilenameInf, std::ios::out | std::ios::binary | std::ios::app);
	if (this->m_fFileInf.is_open() == false) {
		std::wcout << L"Failed to open " + this->m_sFolder + this->m_sFilenameInf << std::endl;
		errors = true;
	}
	this->m_fFileErr.imbue(wloc);
	this->m_fFileErr.open(this->m_sFolder + L"\\" + this->m_sFilenameErr, std::ios::out | std::ios::binary | std::ios::app);
	if (this->m_fFileErr.is_open() == false) {
		std::wcout << L"Failed to open " + this->m_sFolder + this->m_sFilenameErr << std::endl;
		errors = true;
	}

	if (!errors)
		this->m_bInitialized = true;

	return this->m_bInitialized;
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