#pragma once
#include <map>

class Options
{
	typedef std::map<std::string, std::string> OptionCollection;
	OptionCollection options;

public:
	Options();
	~Options();

	int load(std::string filename);
	std::string getString(std::string opt_name);
	int getInt(std::string opt_name);
};

