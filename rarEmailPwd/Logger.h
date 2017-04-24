#pragma once
#include <codecvt>

#include "utility.h"

class Logger
{
	bool m_bInitialized;

	std::wofstream m_fFileInf;
	std::wofstream m_fFileErr;
	bool m_bVerbose;

	UINT m_uiCP; // Console Output Code Page

public:
	Logger(std::wstring folder, std::wstring filenameInf, std::wstring filinameErr, bool verbose, const UINT &encoding);
	~Logger();
	const bool isInitialized();
	void d(std::wstring TAG, std::wstring msg);
	void e(std::wstring TAG, std::wstring msg);
};

