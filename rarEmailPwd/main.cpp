#include "CSmtp\CSmtp.h"
#include "Logger.h"
#include "null_wcodecvt.h"
#include "Options.h"
#include "Report.h"
#include "utility.h"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <forward_list>
#include <io.h>
#include <iomanip>
#include <list>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <windows.h>

bool validateOptions(Options &options);
int getFolders(std::string sDirsListPath, std::forward_list<std::wstring> &folders, const UINT &encoding, Logger &Log);
std::wifstream::pos_type calculateTotalSizeOfArchive(std::string sBackupFolder, std::wstring sArchiveName, const UINT &encoding, Logger &Log);
int SendEmail(std::string sSmtpHost, std::string sSmtpUser, std::string sSmtpPassword, std::string sSendTo,
	std::string sTitle, std::string sMessage, std::forward_list<std::wstring>& attachments, Logger &Log, const UINT &encoding);

int wmain(int argc, wchar_t* argv[]) {
	srand(time(NULL));

	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
	UINT nCCP = GetConsoleOutputCP();
	int nWCP = GetACP();

	null_wcodecvt wcodec(1);
	std::locale wloc(std::locale::classic(), &wcodec);

	std::wstring sOptionsFile;
	if (argc < 3) {
		std::wcout << L"Usage: -o \"path to options file\"" << std::endl;
		return 0;
	}
	else {
		for (int i = 1; i < argc; i++) {
			if (i + 1 != argc) {
				if (!wcscmp(argv[i], L"-o")) {
					sOptionsFile = argv[i + 1];
				}
				else {
					std::wcout << L"Not enough or invalid arguments. Please, try again." << std::endl;
					return -1;
				}
			}
		}
	}

	// Loading options from file
	Options options;
	if (options.load(std::string(sOptionsFile.begin(), sOptionsFile.end())) == -1) {
		std::wcout << L"Failed to open options file." << std::endl;
		return -1;
	}

	if (validateOptions(options)) {
		std::wcout << L"Options file is invalid. Some of necessary options not found." << std::endl;
		return -1;
	}

	// !-- INITIALIZE LOGGER

	Logger Log;

	// Reading options for Logger
	std::wstring sLogPath = strConvert(options.getString("log_path"), nCCP);
	if (!sLogPath.empty()) {
		Log.setFolder(sLogPath);
	}
	std::wstring sLogInfoName = strConvert(options.getString("log_info_name"), nCCP);
	if (!sLogInfoName.empty()) {
		Log.setFilenameInf(sLogInfoName);
	}
	std::wstring sLogErrorName = strConvert(options.getString("log_error_name"), nCCP);
	if (!sLogErrorName.empty()) {
		Log.setFilenameErr(sLogErrorName);
	}
	bool bVerbose = options.getInt("verbose");
	Log.setVerbose(bVerbose);
	Log.setEncoding(nCCP);

	// Initializing Logger instance
	if (!Log.init()) {
		std::wcout << L"Failed to initialize Logger instance." << std::endl;
		return -1;
	}

	// !-- SET UP EMAILS

	int nSendEmails = options.getInt("send_emails");
	if (!nSendEmails) {
		nSendEmails = 2;
	}
	// If email sending is turned on then
	// we need to check if there are needed options
	if (nSendEmails == 2) {
		if (options.getString("smtp_host").empty()
			|| options.getString("smtp_user").empty()
			|| options.getString("smtp_pass").empty()
			|| options.getString("smtp_emailTo").empty()
			|| options.getString("smtp_emailAdmin").empty()) {
			std::wcout << L"Options file is invalid. Provide smtp parameters OR set send_emails to 1." << std::endl;
			return -1;
		}
	}

	// !-- SET UP REPORTS

	// Create a folder for reports inside logs folder
	std::wstring sReportPath = sLogPath + L"\\reports";
	std::wstring s = L"if not exist \"" + sReportPath + L"\" mkdir \"" + sReportPath + L"\"";
	system(std::string(s.begin(), s.end()).c_str());
	// Create Report var
	Report report;

	// !-- ADDITIONAL OPTIONS
	// option for ignoring previous backups (a.k.a do full backup)
	int nForceFull = options.getInt("force_full");

	Log.d(L"rarEmailPwd.main", L"Initialization successful!");

	// !-- INITIALIZATION SUCCESSFUL

	// Open fDirsList to get root folders
	// then add all folders inside root folders as well as
	// root folder to the set to process
	std::forward_list<std::wstring> folders;
	int folderNumber = getFolders(options.getString("dirs_list_path"), folders, nWCP, Log);
	folders.reverse();


	std::wstring sBackupList = L"backup_list.txt";
	if (!nForceFull) {
		// Write UTF_BOM\r\n in the beginning of the file
		// to prevent the bug when first folder written to file
		// gets 2 full backups instead of 1 full + 1 diff
		// AND to create better view of the file
		std::wofstream fBackupList;
		fBackupList.imbue(wloc);
		fBackupList.open(
			L".\\" + sBackupList,
			std::ios::out | std::ios::binary | std::ios::app
		);
		if (fBackupList.is_open())
		{
			fBackupList << UTF_BOM << wendl;
			fBackupList.close();
		}
	}

	std::size_t i = 1;
	for (std::wstring sFolderPath : folders) {
		time_t time_start;
		time(&time_start);
		std::string sTimeStart = getTime(0);

		bool bFlag = false;
		if (sFolderPath[sFolderPath.length() - 1] == '*') {
			bFlag = true;
			sFolderPath = sFolderPath.substr(0, sFolderPath.length() - 1);
		}

		// Constructing archive name
		size_t pos = sFolderPath.find(L"\\", 2);
		std::wstring sIP = sFolderPath.substr(2, pos - 2);
		replace(sIP, L".", L"_");
		std::wstring sDisk = sFolderPath.substr(pos + 1, 1);
		std::wstring sFolder = sFolderPath.substr(pos + 4);
		replace(sFolder, L"/", L"_");
		replace(sFolder, L"\\", L"_");
		replace(sFolder, L" ", L"_");
		std::wstring sArchiveName = sIP + L"_" + sDisk + L"_" + sFolder;

		// Check the last backup date
		// initialize the mode (full/diff backup)
		int nBackupMode = -1; // 0 - full, diff - 1
		double n_secs = 0;

		if (!nForceFull) {
			std::wifstream fBackupList;
			fBackupList.imbue(wloc);
			fBackupList.open(L".\\" + sBackupList, std::ios::in | std::ios::binary);
			if (fBackupList.is_open())
			{
				std::wstring line;
				int n_days = -1;
				int n_full_days = -1;
				int n_diff_days = -1;
				while (getline(fBackupList, line)) {
					std::wstring sBackupName = line.substr(0, line.find(L" "));
					std::wstring sBackupDate = line.substr(line.find(L" ") + 1);
					std::wstring sBackupMode = sBackupDate.substr(sBackupDate.find(L" ") + 1);
					sBackupMode = sBackupMode.substr(0, sBackupMode.length() - 1);
					sBackupDate = sBackupDate.substr(0, sBackupDate.find(L" "));
					if (sBackupName == sArchiveName) {
						struct tm* date = convertStrToTime(std::string(sBackupDate.begin(), sBackupDate.end()), true);
						time_t now;
						time(&now);
						time_t dt = mktime(date);
						n_secs = difftime(now, dt);
						n_days = (int)n_secs / 3600 / 24;
						if (sBackupMode == L"full") {
							n_full_days = n_days;
						}
						else if (sBackupMode == L"diff") {
							n_diff_days = n_days;
						}
					}
				}
				fBackupList.close();

				if (n_full_days == -1) {
					Log.d(L"rarEmailPwd.main", L"This is first backup of \""
						+ sFolderPath
						+ L"\"");
					nBackupMode = 0;
				}
				else if (n_full_days >= options.getInt("full_n_days")) {
					nBackupMode = 0;
					Log.d(L"rarEmailPwd.main", L"Last full archivation of \""
						+ sFolderPath
						+ L"\" was " + std::to_wstring(n_full_days) + L" days ago");
					Log.d(L"rarEmailPwd.main", L"FULL Backup mode activated");
				}
				else if (n_diff_days == -1 || n_diff_days >= options.getInt("diff_n_days")) {
					nBackupMode = 1;
					Log.d(L"rarEmailPwd.main", L"Last full archivation of \""
						+ sFolderPath
						+ L"\" was " + std::to_wstring(n_full_days) + L" days ago");
					Log.d(L"rarEmailPwd.main", L"DIFF Backup mode activated");
				}
				else if (n_full_days < options.getInt("full_n_days") && n_diff_days < options.getInt("diff_n_days")) {
					Log.d(L"rarEmailPwd.main", L"Backup not needed \""
						+ sFolderPath
						+ L"\"");
					continue;
				}
			}
		}
		else {
			nBackupMode = 0;
			Log.d(L"rarEmailPwd.main", L"FORCED FULL BACKUP");
			Log.d(L"rarEmailPwd.main", L"FULL Backup mode activated");
		}

		std::wstring wsResult;
		if (!nForceFull) {
			// Check all backups in backup folder
			// if they are outdated -> delete them
			Log.d(L"rarEmailPwd.main", L"Starting forfiles.exe to find and delete outdated archives");
			std::wstring sForfiles;
			std::string sResult;
			if (nBackupMode == 0) {
				sForfiles = L"forfiles.exe /p " + strConvert(options.getString("backup_path"), nCCP) +
					L" /s /m " + sArchiveName + L"_full_*.rar /d -" + strConvert(options.getString("full_keep_n_days"), nCCP)
					+ L" /c \"cmd /c del /q /f @file\"";
				Log.d(L"rarEmailPwd.main", sForfiles);
			}
			else if (nBackupMode == 1) {
				sForfiles = L"forfiles.exe /p " + strConvert(options.getString("backup_path"), nCCP) +
					L" /s /m " + sArchiveName + L"_diff_*.rar /d -" + strConvert(options.getString("diff_keep_n_days"), nCCP)
					+ L" /c \"cmd /c del /q /f @file\"";
				Log.d(L"rarEmailPwd.main", sForfiles);
			}
			exec(sForfiles, wsResult, nCCP);
			if (!wsResult.empty()) {
				wsResult.erase(std::remove(wsResult.begin(), wsResult.end(), '\n'), wsResult.end());
				Log.d(L"rarEmailPwd.main", wsResult);
			}
		}

		std::wstring wsInPath = sFolderPath;
		std::wstring wsOutPath = strConvert(options.getString("backup_path"), nCCP) + L"\\"
			+ sArchiveName + L"_" + (nBackupMode ? L"diff" : L"full");

		std::wstring wsCommand =
			strConvert(options.getString("rar_path"), nCCP) + L" a "
			+ L"\"" + wsOutPath + L".tmp\" "	// out path
			+ L"\"" + wsInPath + L"\" "				// in path
			+ strConvert(options.getString("rar_argum"), nCCP);
		// add .tmp in the end to calculate resulting filesize later,
		// .tmp will be replaced with .rar after completion

		if (bFlag) {
			wsCommand += L" -r-";
		}

		if (nBackupMode == 0) {
			wsCommand += L" -m5 -ac";
		}
		else if (nBackupMode == 1) {
			wsCommand += L" -m4 -ao";
		}

		Log.d(L"rarEmailPwd.main", wsCommand);

		// Adding password for archive encryption
		std::string sPassword = randomStrGen(options.getInt("pwd_length"));
		wsCommand = wsCommand + strConvert(" -hp\"" + sPassword + "\"", nCCP);

		// Sending Email with password before archivation start
		// (to make sure password is available in case of program failure)
		Log.d(L"rarEmailPwd.main", L"Sending email with password");
		if (nSendEmails == 2) {
			std::string sMessage = std::string(wsOutPath.begin(), wsOutPath.end()) + " \n " + sPassword + "\n";

			if (SendEmail(
				options.getString("smtp_host"),
				options.getString("smtp_user"),
				options.getString("smtp_pass"),
				options.getString("smtp_emailTo"),
				std::string(sArchiveName.begin(), sArchiveName.end()) + " " + (nBackupMode == 0 ? "FULL" : "DIFF"),
				sMessage,
				std::forward_list<std::wstring>(),
				Log,
				nCCP)) {
				Log.d(L"rarEmailPwd.sendEmail", L"ERROR Email was not sent.");
			}
		}
		
		/// TODO: REMOVE AFTER TESTING
		//Log.d(L"rarEmailPwd.main", wsCommand);

		Log.d(L"rarEmailPwd.main", L"Starting WinRAR. WinRAR logs will be available in separate files.");

		std::wstring sWinRARLogAllName = L"WinRAR_" + sArchiveName + L"_" + (nBackupMode ? L"diff" : L"full") + L"_all.log";
		Logger WinRARLog;
		WinRARLog.setFolder(sLogPath);
		WinRARLog.setFilenameInf(sWinRARLogAllName);
		WinRARLog.setFilenameErr(sWinRARLogAllName);
		WinRARLog.setVerbose(bVerbose);
		WinRARLog.setEncoding(nCCP);
		if (!WinRARLog.init()) {
			std::wcout << L"Failed to initialize Logger instance (WinRARLog)." << std::endl;
			return -1;
		}

		DWORD nResult = exec(wsCommand, wsResult, nCCP);
		WinRARLog.d(L"rarEmailPwd.main", L"WinRAR has finished");

		replace(wsResult, L"\n", L"\r\n");
		wsResult = L"\r\n" + wsResult;

		WinRARLog.d(L"rarEmailPwd.main", wsResult);

		time_t time_end;
		time(&time_end);
		double secs = difftime(time_end, time_start);
		std::string sDuration = prettyTimeString(secs);
		if (nResult == 0 || nResult == 1) {
			std::wstring strWarnings = (nResult == 1 ? L" (with warnings)" : L"");
			Log.d(L"rarEmailPwd.main", L"Archivation successful" + strWarnings);

			// Calcalating size of the archive (total)
			std::ifstream::pos_type nFilesize = calculateTotalSizeOfArchive(options.getString("backup_path"), sArchiveName, nCCP, Log);
			std::string sFilesize = prettyFSString(nFilesize);

			Log.d(L"rarEmailPwd.main", L"Filesize: " + strConvert(sFilesize, nCCP));
			Log.d(L"rarEmailPwd.main", L"Duration: " + strConvert(sDuration, nCCP));

			// Sending email
			if (nSendEmails == 2) {
				std::string sMessage = std::string(wsOutPath.begin(), wsOutPath.end()) + " \n " + sPassword + "\n";

				sMessage += (nResult == 1 ? " (with warnings)\n" : "\n");
				sMessage += "Time (start): " + sTimeStart + "\n";
				sMessage += "Time (finish): " + getTime(0) + "\n";
				sMessage += "Duration: " + sDuration + "\n";
				sMessage += "Filesize: " + sFilesize + "\n";

				if (SendEmail(
					options.getString("smtp_host"),
					options.getString("smtp_user"),
					options.getString("smtp_pass"),
					options.getString("smtp_emailTo"),
					std::string(sArchiveName.begin(), sArchiveName.end()) + " " + (nBackupMode == 0 ? "FULL" : "DIFF"),
					sMessage,
					std::forward_list<std::wstring>(),
					Log,
					nCCP)) {
					Log.d(L"rarEmailPwd.sendEmail", L"ERROR Email was not sent.");
				}
			}

			std::wofstream fBackupList;
			fBackupList.imbue(wloc);
			fBackupList.open(
				L".\\" + sBackupList,
				std::ios::out | std::ios::binary | std::ios::app
			);
			if (fBackupList.is_open())
			{
				std::wstring l = sArchiveName + L" " + strConvert(getTime(2), nCCP);
				if (nBackupMode == 0) {
					l += L" full";
				}
				else if (nBackupMode == 1) {
					l += L" diff";
				}
				//fBackupList << UTF_BOM << wendl;
				fBackupList << l << wendl;
				fBackupList.close();
			}

			// Add report item
			ReportItem item = {
				1,
				sArchiveName,
				(nBackupMode == 0 ? L"full" : L"diff"),
				strConvert(sTimeStart, nCCP),
				strConvert(getTime(0), nCCP),
				strConvert(sDuration, nCCP),
				strConvert(sFilesize, nCCP)
			};
			report.addItem(item);
		}
		else if (nResult == 10) {
			Log.d(L"rarEmailPwd.main", L"Empty folder! Skipped");

			// Add report item
			ReportItem item = {
				2,
				sArchiveName,
				(nBackupMode == 0 ? L"full" : L"diff"),
				strConvert(sTimeStart, nCCP),
				strConvert(getTime(0), nCCP),
				strConvert(sDuration, nCCP),
				L"-"
			};
			report.addItem(item);
		}
		else {
			Log.d(L"rarEmailPwd.main", L"Archivation failed");

			// Add report item
			ReportItem item = {
				0,
				sArchiveName,
				(nBackupMode == 0 ? L"full" : L"diff"),
				strConvert(sTimeStart, nCCP),
				strConvert(getTime(0), nCCP),
				strConvert(sDuration, nCCP),
				L"-"
			};
			report.addItem(item);
		}

		i++;
	}

	if (options.getInt("notify_level") == 2 || options.getInt("notify_level") == 3) {
		std::wstring sReportFilename = sReportPath + L"\\" + L"report_" + strConvert(getTime(3), nCCP) + L".html";
		report.createReport(sReportFilename, strConvert(getTime(2), nCCP));

		std::forward_list<std::wstring> attachments;
		attachments.push_front(sReportFilename);

		std::string sMessage = "Your report is in attachement\n";

		if (options.getInt("notify_level") == 3) {
			if (SendEmail(
				options.getString("smtp_host"),
				options.getString("smtp_user"),
				options.getString("smtp_pass"),
				options.getString("smtp_emailAdmin"),
				"RarBackup Report " + getTime(3),
				sMessage,
				attachments,
				Log,
				nCCP)) {
				Log.d(L"rarEmailPwd.sendEmail", L"ERROR Report Email was not sent.");
			}
		}
	}
	return 0;
}

bool validateOptions(Options &options) {
	// 1 - invalid, 0 - valid
	return !options.getInt("full_n_days")
		|| !options.getInt("full_keep_n_days")
		|| !options.getInt("diff_keep_n_days")
		|| options.getString("rar_path").empty()
		|| options.getString("rar_argum").empty()
		|| !options.getInt("pwd_length")
		|| !options.getInt("num_of_tries")
		|| options.getString("dirs_list_path").empty()
		|| options.getString("backup_path").empty()
		|| !options.getInt("notify_level");
}

int getFolders(std::string sDirsListPath, std::forward_list<std::wstring> &folders, const UINT &encoding, Logger &Log) {
	// remember locale
	std::string oldLocale = setlocale(LC_ALL, NULL);
	// set current environment locale
	setlocale(LC_ALL, "");

	int cnt = 0;

	std::wifstream fDirsList;
	fDirsList.open(sDirsListPath.c_str());
	if (fDirsList.is_open()) {
		std::wstring line;
		while (getline(fDirsList, line)) {
			std::string sRootFolderPath = std::string(line.begin(), line.end());

			DIR *dir = opendir(sRootFolderPath.c_str());
			if (dir) {
				folders.push_front(line + L"\\*");
				cnt++;
				struct dirent *entry = readdir(dir);
				while (entry != NULL) {
					if (entry->d_type == DT_DIR
						&& strcmp(entry->d_name, ".")
						&& strcmp(entry->d_name, "..")) {
						std::wstring sEncodedName = strConvert(sRootFolderPath, encoding) + L"\\" + strConvert(entry->d_name, encoding);
						folders.push_front(sEncodedName);
						cnt++;
					}
					entry = readdir(dir);
				}
				closedir(dir);
			}
			else {
				Log.e(L"rarEmailPwd.main", L"ERROR root folder does not exist " + strConvert(sRootFolderPath, encoding));
			}
		}
		fDirsList.close();
	}

	// restore locale
	setlocale(LC_ALL, oldLocale.c_str());

	return cnt;
}

std::wifstream::pos_type calculateTotalSizeOfArchive(std::string sBackupFolder, std::wstring sArchiveName, const UINT &encoding, Logger &Log) {
	std::wifstream::pos_type nTotalSize = 0;

	// remember locale
	std::string oldLocale = setlocale(LC_ALL, NULL);
	// set current environment locale
	setlocale(LC_ALL, "");

	std::wstring tmpExt = L".tmp";
	std::wstring rarExt = L".rar";

	DIR *dir = opendir(sBackupFolder.c_str());

	struct dirent *entry = readdir(dir);

	while (entry != NULL)
	{
		if (strcmp(entry->d_name, ".")
			&& strcmp(entry->d_name, "..")) {
			std::wstring sName = strConvert(entry->d_name);
			// If filename in backups folder begins with archiveName
			// and contains tmpExt then take it into calculation
			if (hasPrefix(sName, sArchiveName)
			 && hasSubstr(sName, tmpExt)) {
				// Create new name
				std::wstring sNewFilename = sName.substr(0, sName.find(tmpExt));
				if (sName.find(rarExt) != std::wstring::npos) {
					std::wstring secondPart = sName.substr(sName.find(tmpExt) + tmpExt.size());
					secondPart = secondPart.substr(0, secondPart.find(rarExt));
					sNewFilename += secondPart;
				}
				sNewFilename += rarExt;

				sName = strConvert(sBackupFolder) + L"\\" + sName;
				nTotalSize += filesize(sName);
				// remove .tmp from the end of filename
				std::wstring wsCommand = L"rename \"" + sName + L"\" \"" + sNewFilename + L"\"";
				std::wstring wsResult;
				DWORD nResult = exec(wsCommand, wsResult, encoding);
				if (nResult) { // if error when renaming (try deleting old file)
					wsCommand = L"del \"" + strConvert(sBackupFolder) + L"\\" + sNewFilename + L"\"";
					nResult = exec(wsCommand, wsResult, encoding);
					if (nResult != 12 && nResult != 16) { // deletion successful
						wsCommand = L"rename \"" + sName + L"\" \"" + sNewFilename + L"\"";
						exec(wsCommand, wsResult, encoding);
					}
				}
			}
		}

		entry = readdir(dir);
	}

	// restore locale
	setlocale(LC_ALL, oldLocale.c_str());

	closedir(dir);

	return nTotalSize;
}

int SendEmail(std::string sSmtpHost, std::string sSmtpUser, std::string sSmtpPassword, std::string sSendTo,
	std::string sTitle, std::string sMessage, std::forward_list<std::wstring>& attachments, Logger &Log, const UINT &encoding = CP_ACP)
{
	Log.d(L"rarEmailPwd.sendEmail", L"Sending email...");
	CSmtp mail;
	bool bError = false;
	try
	{
		mail.SetSMTPServer(sSmtpHost.c_str(), 25);
		mail.SetLogin(sSmtpUser.c_str());
		mail.SetPassword(sSmtpPassword.c_str());
		mail.SetSenderName(sSmtpUser.c_str());
		mail.SetSenderMail(sSmtpUser.c_str());
		mail.SetReplyTo(sSmtpUser.c_str());
		mail.SetSubject(sTitle.c_str());
		mail.AddRecipient(sSendTo.c_str());
		mail.SetXPriority(XPRIORITY_NORMAL);
		mail.SetXMailer("RarBackup Emailer (CSmtp)");

		// Create message
		std::stringstream ss(sMessage);
		std::string to;

		if (sMessage.c_str() != NULL)
		{
			while (std::getline(ss, to, '\n')) {
				mail.AddMsgLine(to.c_str());
			}
		}

		// Adding attachments
		for (std::wstring sAttachmentFile : attachments) {
			mail.AddAttachment(std::string(sAttachmentFile.begin(), sAttachmentFile.end()).c_str());
		}

		// Send email
		mail.Send();
	}
	catch (ECSmtp e)
	{
		// code to get unsent message
		//std::string sMessage = "";
		//int nLines = mail.GetMsgLines();
		//for (int i = 0; i < nLines; i++) {
		//	std::string line = mail.GetMsgLineText(i);
		//	sMessage += line + "\r\n";
		//}

		Log.e(L"rarEmailPwd.sendEmail", L"Error: " + strConvert(e.GetErrorText(), encoding));
		//Log.e(L"rarEmailPwd.sendEmail", L"Unsent message:\r\n\r\n" + strConvert(sMessage, encoding) + L"\r\n");
		bError = true;
	}

	if (!bError)
	{
		Log.d(L"rarEmailPwd.sendEmail", L"Mail was sent successfully");
		return 0;
	}
	else
		return -1;
	return 0;
}