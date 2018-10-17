#include "Logger.h"

using namespace std;

Logger::Logger(char * filename) {
	if (filename) {
		file.open(filename);
		if (!file.is_open()) {
			cerr << "[IOException] Cannot open file " << filename;
			useCout = true;
		}
		useCout = false;
	}
	else {
		useCout = true;
	}
}


Logger::~Logger() {
	if (useCout) { file.close(); }
}

void Logger::log(string text) {
	useCout ? file << text << endl : cout << text << endl;
}