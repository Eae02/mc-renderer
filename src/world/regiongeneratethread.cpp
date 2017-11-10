#include "regiongeneratethread.h"

namespace MCR
{
	RegionGenerateThread::RegionGenerateThread(size_t numThreads)
	{
		for (size_t i = 0; i < numThreads; i++)
		{
			m_threads.emplace_back(&RegionGenerateThread::ThreadTarget, this);
			
			SetThreadDesc(m_threads.back().get_id(), "RegionGen" + std::to_string(i));
		}
	}
	
	RegionGenerateThread::~RegionGenerateThread()
	{
		{
			std::lock_guard<std::mutex> lock(m_inputMutex);
			m_exit = true;
		}
		
		m_signal.notify_all();
		for (std::thread& thread : m_threads)
		{
			thread.join();
		}
	}
	
	void RegionGenerateThread::EndRegistering()
	{
		m_inputMutex.unlock();
		
		if (m_anyEnqueued)
		{
			m_signal.notify_all();
		}
	}
	
	void RegionGenerateThread::ThreadTarget()
	{
		while (true)
		{
			std::unique_lock<std::mutex> inputLock(m_inputMutex);
			
			m_signal.wait(inputLock, [&] { return !m_regionsToGenerate.empty() || m_exit; });
			
			if (m_exit)
				break;
			
			//Selects the closest region to the camera for generation.
			long selectedIndex = -1;
			uint64_t selectedRegionDistFromCamera = 0;
			for (size_t i = 0; i < m_regionsToGenerate.size(); i++)
			{
				uint64_t distFromCamera = RegionCoordinate::DistanceSq(m_regionsToGenerate[i], m_cameraRegion);
				if (selectedRegionDistFromCamera > distFromCamera || selectedIndex == -1)
				{
					selectedIndex = i;
					selectedRegionDistFromCamera = distFromCamera;
				}
			}
			
			RegionCoordinate regionCoord = m_regionsToGenerate[selectedIndex];
			m_regionsToGenerate[selectedIndex] = m_regionsToGenerate.back();
			m_regionsToGenerate.pop_back();
			
			inputLock.unlock();
			
#ifdef MCR_REGION_LOG
			Log("Generating (", regionCoord.x, ", ", regionCoord.z, ")");
#endif
			
			NewRegion newRegion(std::make_shared<Region>(regionCoord.x, regionCoord.z));
			
			m_generator.Generate(*newRegion.m_region, newRegion.m_hasWater);
			
#ifdef MCR_REGION_LOG
			Log("Generated (", regionCoord.x, ", ", regionCoord.z, ")");
#endif
			
			std::lock_guard<std::mutex> outputLock(m_outputMutex);
			
			m_generatedRegions.push_back(std::move(newRegion));
		}
	}
}
