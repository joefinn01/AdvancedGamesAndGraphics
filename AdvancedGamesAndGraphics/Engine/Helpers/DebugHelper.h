#pragma once

#include <utility>
#include <string>

enum class LogLevel
{
	VERBOSE = 0,
	WARNING,
	ERROR
};

#if _DEBUG

#define LOG_VERBOSE(tag, format, ...) DebugHelper::Log((LogLevel)0, tag, format, __VA_ARGS__);
#define LOG_WARNING(tag, format, ...) DebugHelper::Log((LogLevel)1, tag, format, __VA_ARGS__);
#define LOG_ERROR(tag, format, ...) DebugHelper::Log((LogLevel)2, tag, format, __VA_ARGS__);

#else

#define LOG_VERBOSE(tag, format, ...)
#define LOG_WARNING(tag, format, ...)
#define LOG_ERROR(tag, format, ...)

#endif

typedef const std::string Tag;

class DebugHelper
{
public:
	static void Log(const LogLevel& logLevel, const std::string& sTag, const std::string& sText, ...);
protected:

private:

};

