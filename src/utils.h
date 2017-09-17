#pragma once

#define MAKE_RANGE(x) std::begin(x), std::end(x)

#ifdef __cpp_if_constexpr
#define constexpr_if if constexpr
#else
#define constexpr_if if
#endif

#include <ostream>
#include <gsl/span>
#include <mutex>
#include <thread>
#include <iomanip>
#include <chrono>
#include <string_view>

#include "filesystem.h"

namespace MCR
{
	template <typename T>
	inline gsl::span<T> SingleElementSpan(T& element)
	{
		return { &element, 1 };
	}
	
	bool IsAttachedToDebugger();
	
	namespace Detail
	{
		extern std::mutex logStreamMutex;
		
		std::ostream& GetLogStream();
		
		template <typename Arg>
		inline void LogImpl(std::ostream& stream, Arg arg)
		{
			stream << arg;
		}
		
		template <typename Arg, typename... Args>
		inline void LogImpl(std::ostream& stream, Arg arg, Args... args)
		{
			stream << arg;
			LogImpl(stream, args...);
		}
	}
	
	void SetThreadDesc(std::thread::id id, std::string description);
	
	std::string_view GetThreadDesc(std::thread::id id);
	
	template <typename... Args>
	void Log(Args... args)
	{
		std::lock_guard<std::mutex> lock(Detail::logStreamMutex);
		
		std::ostream& logStream = Detail::GetLogStream();
		
		logStream << GetThreadDesc(std::this_thread::get_id()) << ": ";
		Detail::LogImpl(logStream, args...);
		logStream << std::endl;
	}
	
	std::vector<char> ReadFileContents(const fs::path& path);
	
	const fs::path& GetResourcePath();
	
	template <typename DataType, typename StreamType>
	inline DataType BinRead(StreamType& stream)
	{
		DataType value;
		stream.read(reinterpret_cast<char*>(&value), sizeof(DataType));
		return value;
	}
	
	template <typename DataType, typename StreamType>
	inline void BinWrite(StreamType& stream, DataType value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(DataType));
	}
	
	template <typename T>
	constexpr inline T RoundToNextMultiple(T value, T multiple)
	{
		T valModMul = value % multiple;
		return valModMul == 0 ? value : (value + multiple - valModMul);
	}
	
	template <typename T>
	constexpr inline T DivRoundUp(T num, T denom)
	{
		return num / denom + (num % denom != 0);
	}
	
	template <typename T, size_t I>
	constexpr inline size_t ArrayLength(T (& array)[I])
	{
		return I;
	}
}
