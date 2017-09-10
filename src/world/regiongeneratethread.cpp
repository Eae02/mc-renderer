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
	
	void RegionGenerateThread::BeginRegistering()
	{
		m_inputMutex.lock();
		m_anyEnqueued = false;
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
			
			RegionCoordinate regionCoord = m_regionsToGenerate.front();
			m_regionsToGenerate.pop();
			
			inputLock.unlock();
			
#ifdef MCR_REGION_LOG
			Log("Generating (", regionCoord.x, ", ", regionCoord.z, ")");
#endif
			
			Region region = m_generator.Generate(regionCoord.x, regionCoord.z);
			
#ifdef MCR_REGION_LOG
			Log("Generated (", regionCoord.x, ", ", regionCoord.z, ")");
#endif
			
			std::lock_guard<std::mutex> outputLock(m_outputMutex);
			
			m_generatedRegions.push_back(std::move(region));
		}
	}
}
