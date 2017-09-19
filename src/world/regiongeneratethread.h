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
		
		inline void BeginRegistering()
		{
			m_inputMutex.lock();
			m_anyEnqueued = false;
		}
		
		void EndRegistering();
		
		//Only call between BeginRegistering and EndRegistering.
		inline void Register(RegionCoordinate coordinate)
		{
			m_regionsToGenerate.push_back(coordinate);
			m_anyEnqueued = true;
		}
		
		//Only call between BeginRegistering and EndRegistering.
		inline void SetCameraRegion(RegionCoordinate coordinate)
		{
			m_cameraRegion = coordinate;
		}
		
		template <typename CallbackTp>
		bool IterateGeneratedRegions(CallbackTp callback)
		{
			std::lock_guard<std::mutex> lock(m_outputMutex);
			
			if (m_generatedRegions.empty())
				return false;
			
			for (std::shared_ptr<Region>& region : m_generatedRegions)
			{
				callback(region);
			}
			m_generatedRegions.clear();
			
			return true;
		}
		
		inline void ProcessFutureRegions(class WorldManager& worldManager)
		{
			m_generator.ProcessFutureRegions(worldManager);
		}
		
	private:
		void ThreadTarget();
		
		WorldGenerator m_generator;
		
		std::mutex m_inputMutex;
		std::mutex m_outputMutex;
		
		bool m_anyEnqueued = false;
		bool m_exit = false;
		
		RegionCoordinate m_cameraRegion;
		
		std::vector<RegionCoordinate> m_regionsToGenerate;
		
		std::condition_variable m_signal;
		
		std::vector<std::shared_ptr<Region>> m_generatedRegions;
		
		std::vector<std::thread> m_threads;
	};
}
