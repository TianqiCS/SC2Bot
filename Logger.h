#pragma once

#include <iostream>
#include <fstream>
#include <string>

class Logger
{
public:
	Logger(char* filename = NULL);
	~Logger();
	void log(std::string text);
//	void create(const std::string text);
//	void destroy(const std::string text);


private:
	std::ofstream file;
	bool useCout;
};

