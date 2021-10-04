#include "DebugHelper.h"

#include <stdio.h>
#include <iostream>
#include <cstdarg>

#include <sstream>

#define BUFFER_SIZE 256

const std::string ksVerboseTag = "VERBOSE: ";
const std::string ksWarningTag = "WARNING: ";
const std::string ksErrorTag = "ERROR: ";

void DebugHelper::Log(const LogLevel& logLevel, const std::string& sTag, const std::string& sText, ...)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	std::stringstream ss;

	ss << sTag << "::";

	switch (logLevel)
	{
	case LogLevel::VERBOSE:
		ss << ksVerboseTag;
		break;

	case LogLevel::WARNING:
		ss << ksWarningTag;
		break;

	case LogLevel::ERROR:
		ss << ksErrorTag;
		break;

	default:
		break;
	}

	ss << sText;

	//pass in varidic arguments and output to the console.
	va_list args;
	va_start(args, sText);
	vsprintf_s(buffer, ss.str().c_str(), args);
	fprintf(stdout, buffer);
	std::cout << std::endl;
	va_end(args);
}
