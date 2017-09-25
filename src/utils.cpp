#include "utils.h"

#include <iostream>
#include <unordered_map>
#include <cstring>
#include <fstream>

#if defined(__linux__)

#include <libgen.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <shlobj.h>

#endif

namespace MCR
{
	std::mutex Detail::logStreamMutex;
	
	std::ostream& Detail::GetLogStream()
	{
		return std::cout;
	}
	
	const fs::path& GetResourcePath()
	{
		static fs::path resourcePath;
		
		if (resourcePath.empty())
		{
#if defined(__linux__)
			char exePathOut[PATH_MAX];
			long numBytesWritten = readlink("/proc/self/exe", exePathOut, PATH_MAX);
			
			if (numBytesWritten == -1)
			{
				throw std::runtime_error("Error getting path to executable.");
			}
			exePathOut[numBytesWritten] = '\0';
			
			dirname(exePathOut);
			resourcePath = fs::path(exePathOut) / "res";
#elif defined(_WIN32)
			TCHAR exePathOut[MAX_PATH];
			GetModuleFileName(nullptr, exePathOut, MAX_PATH);
			
			resourcePath = fs::path(exePathOut).parent_path() / "res";
#endif
		}
		
		return resourcePath;
	}
	
	std::unordered_map<std::thread::id, std::string> threadDescriptions;
	
	void SetThreadDesc(std::thread::id id, std::string description)
	{
		threadDescriptions[id] = std::move(description);
	}
	
	std::string_view GetThreadDesc(std::thread::id id)
	{
		auto descIt = threadDescriptions.find(id);
		
		if (descIt != threadDescriptions.end())
		{
			return descIt->second;
		}
		
		return "Unnamed Thread";
	}
	
	std::vector<char> ReadFileContents(const fs::path& path)
	{
		size_t fileSize = fs::file_size(path);
		
		std::ifstream stream(path, std::ios::binary);
		if (!stream)
		{
			throw std::runtime_error("Error opening file for reading: '" + path.string() + "'.");
		}
		
		std::vector<char> result(fileSize);
		stream.read(result.data(), result.size());
		
		return result;
	}
	
	static bool checkAttachedToDebugger = true;
	static bool isAttachedToDebugger = false;
	
	bool IsAttachedToDebugger()
	{
		if (checkAttachedToDebugger)
		{
#if defined(__linux__)
			std::array<char, 1024> buffer;
			
			int statusFD = open("/proc/self/status", O_RDONLY);
			if (statusFD == -1)
				return false;
			
			long numRead = read(statusFD, buffer.data(), buffer.size() - 1);
			
			if (numRead > 0)
			{
				static const char tracerPid[] = "TracerPid:";
				char* tracerPidPos;
				
				buffer[numRead] = '\0';
				tracerPidPos = std::strstr(buffer.data(), tracerPid);
				if (tracerPidPos)
				{
					isAttachedToDebugger = std::atoi(tracerPidPos + sizeof(tracerPid) - 1) != 0;
				}
			}
#elif defined(_WIN32)
			isAttachedToDebugger = IsDebuggerPresent();
#else
			isAttachedToDebugger = false;
#endif
			
			checkAttachedToDebugger = false;
		}
		
		return isAttachedToDebugger;
	}
}
