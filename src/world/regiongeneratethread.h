#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "region.h"
#include "worldgenerator.h"

namespace MCR
{
	class RegionGenerateThread final
	{
	public:
		explicit RegionGenerateThread(size_t numThreads);
		~RegionGenerateThread();
		
		void BeginRegistering();
		
		void EndRegistering();
		
		//Only call between BeginPolling and EndPolling.
		inline void Register(RegionCoordinate coordinate)
		{
			m_regionsToGenerate.push(coordinate);
			m_anyEnqueued = true;
		}
		
		template <typename CallbackTp>
		void IterateGeneratedRegions(CallbackTp callback)
		{
			std::lock_guard<std::mutex> lock(m_outputMutex);
			
			for (Region& region : m_generatedRegions)
			{
				callback(region);
			}
			m_generatedRegions.clear();
		}
		
	private:
		void ThreadTarget();
		
		WorldGenerator m_generator;
		
		std::mutex m_inputMutex;
		std::mutex m_outputMutex;
		
		bool m_anyEnqueued = false;
		bool m_exit = false;
		
		std::queue<RegionCoordinate> m_regionsToGenerate;
		
		std::condition_variable m_signal;
		
		std::vector<Region> m_generatedRegions;
		
		std::vector<std::thread> m_threads;
	};
}
