#pragma once

#include <string>

enum class LogLevel
{
	VERBOSE = 0,
	WARNING,
	ERROR
};

#if _DEBUG

#define LOG_VERBOSE(tag, format, ...) DebugHelper::Log(LogLevel::VERBOSE, tag, format, __VA_ARGS__);
#define LOG_WARNING(tag, format, ...) DebugHelper::Log(LogLevel::WARNING, tag, format, __VA_ARGS__);
#define LOG_ERROR(tag, format, ...) DebugHelper::Log(LogLevel::ERROR, tag, format, __VA_ARGS__);

#else

#define LOG_VERBOSE(tag, format, ...)
#define LOG_WARNING(tag, format, ...)
#define LOG_ERROR(tag, format, ...)

#endif

class DebugHelper
{
public:
	static void Log(LogLevel logLevel, const std::string sTag, const std::string sText, ...);
protected:

private:

};

