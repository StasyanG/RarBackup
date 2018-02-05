#include "Options.h"
#include <fstream>
#include <string>

Options::Options() {
}

Options::~Options() {
}

int Options::load(std::string filename) {
	std::ifstream opt_file;
	opt_file.open(filename.c_str());
	if (opt_file.is_open()) {
		std::string line;
		while (getline(opt_file, line)) {
			if (line.length() != 0 && line[0] != '#') {
				std::string opt_name = line.substr(0, line.find("="));
				std::string opt_value = line.substr(line.find("=") + 1);
				options[opt_name] = opt_value;
			}
		}
		opt_file.close();
		return 0;
	}
	else {
		return -1;
	}
}

std::string Options::getString(std::string opt_name) {
	return options.find(opt_name) != options.end() ? options[opt_name] : NULL;
}

int Options::getInt(std::string opt_name) {
	if (options.find(opt_name) != options.end()) {
		char * pEnd;
		long int value = strtol(options[opt_name].c_str(), &pEnd, 10);
		if (value != 0L) {
			return (int)value;
		}
	}

	return NULL;
}