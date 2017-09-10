#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <stack>

#include "region.h"

namespace MCR
{
	class RegionIOThread final
	{
	public:
		explicit RegionIOThread(class World& world);
		~RegionIOThread();
		
		void WaitIdle();
		
		void BeginRegistering();
		
		void EndRegistering();
		
		//Only call between BeginPolling and EndPolling.
		inline void RegisterForSaving(Region region)
		{
			m_regionsToSave.push(std::move(region));
			m_anyEnqueued = true;
		}
		
		//Only call between BeginPolling and EndPolling.
		inline void RegisterForLoading(RegionCoordinate coordinate)
		{
			m_regionsToLoad.push(std::move(coordinate));
			m_anyEnqueued = true;
		}
		
		template <typename CallbackTp>
		void IterateLoadedRegions(CallbackTp callback)
		{
			std::lock_guard<std::mutex> lock(m_outputMutex);
			
			for (Region& region : m_loadedRegions)
			{
				callback(region);
			}
			m_loadedRegions.clear();
		}
		
	private:
		void ThreadTarget();
		
		bool m_exit = false;
		
		bool m_anyEnqueued = false;
		
		bool m_idle = false;
		std::condition_variable m_idleSignal;
		
		class World& m_world;
		
		std::mutex m_inputMutex;
		std::mutex m_outputMutex;
		
		std::queue<Region> m_regionsToSave;
		std::queue<RegionCoordinate> m_regionsToLoad;
		
		std::condition_variable m_taskAvailableSignal;
		
		std::vector<Region> m_loadedRegions;
		
		std::thread m_thread;
	};
}
