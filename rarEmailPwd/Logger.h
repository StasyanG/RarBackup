#pragma once
#include "utility.h"
#include <codecvt>

class Logger
{
	bool m_bInitialized;

	std::wstring m_sFolder;
	std::wstring m_sFilenameInf;
	std::wstring m_sFilenameErr;
	bool m_bVerbose;

	std::wofstream m_fFileInf;
	std::wofstream m_fFileErr;

	UINT m_uiCP; // Console Output Code Page

public:
	Logger();
	~Logger();

	void setDefaultSettings();
	void setFolder(std::wstring folder);
	void setFilenameInf(std::wstring filenameInf);
	void setFilenameErr(std::wstring filinameErr);
	void setVerbose(bool verbose);
	void setEncoding(const UINT &encoding);

	int init();
	const bool isInitialized();

	void d(std::wstring TAG, std::wstring msg);
	void e(std::wstring TAG, std::wstring msg);
};
