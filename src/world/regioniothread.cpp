#include "regioniothread.h"
#include "world.h"

namespace MCR
{
	RegionIOThread::RegionIOThread(World& world)
	    : m_world(world), m_thread(&RegionIOThread::ThreadTarget, this) { }
	
	RegionIOThread::~RegionIOThread()
	{
		{
			std::lock_guard<std::mutex> lock(m_inputMutex);
			m_exit = true;
		}
		
		m_taskAvailableSignal.notify_one();
		m_thread.join();
	}
	
	void RegionIOThread::WaitIdle()
	{
		Log("Waiting for IO thread to finish.");
		
		std::unique_lock<std::mutex> lock(m_inputMutex);
		
		m_idleSignal.wait(lock, [&] { return m_idle; });
		
		Log("IO thread finished.");
	}
	
	void RegionIOThread::BeginRegistering()
	{
		m_inputMutex.lock();
		m_anyEnqueued = false;
	}
	
	void RegionIOThread::EndRegistering()
	{
		if (m_anyEnqueued && m_idle)
		{
			m_taskAvailableSignal.notify_one();
			m_idle = false;
		}
		
		m_inputMutex.unlock();
	}
	
	void RegionIOThread::ThreadTarget()
	{
		SetThreadDesc(std::this_thread::get_id(), "RegionIO");
		
		while (true)
		{
			std::unique_lock<std::mutex> lock(m_inputMutex);
			
			while (m_regionsToSave.empty() && m_regionsToLoad.empty() && !m_exit)
			{
				if (!m_idle)
				{
					m_idle = true;
					m_idleSignal.notify_all();
				}
				
				m_taskAvailableSignal.wait(lock);
			}
			
			if (m_exit && m_regionsToSave.empty())
			{
				break;
			}
			
			std::shared_ptr<const Region> regionToSave;
			if (!m_regionsToSave.empty())
			{
				regionToSave = std::move(m_regionsToSave.front());
				m_regionsToSave.pop();
			}
			
			std::optional<RegionCoordinate> regionToLoad;
			if (!m_regionsToLoad.empty() && !m_exit)
			{
				regionToLoad = m_regionsToLoad.front();
				m_regionsToLoad.pop();
			}
			
			lock.unlock();
			
			if (regionToSave)
			{
#ifdef MCR_REGION_LOG
				Log("Saving (", regionToSave->GetX(), ", ", regionToSave->GetZ(), ")");
#endif
				
				m_world.SaveRegion(*regionToSave);
			}
			
			if (regionToLoad.has_value())
			{
#ifdef MCR_REGION_LOG
				Log("Loading region (", regionToLoad->x, ", ", regionToLoad->z, ")");
#endif
				
				std::shared_ptr<Region> region = std::make_unique<Region>(regionToLoad->x, regionToLoad->z);
				m_world.LoadRegion(*region);
				
#ifdef MCR_REGION_LOG
				Log("Loaded region (", regionToLoad->x, ", ", regionToLoad->z, ")");
#endif
				
				std::lock_guard<std::mutex> loadedRegionsLock(m_outputMutex);
				m_loadedRegions.push_back(std::move(region));
			}
		}
	}
}
