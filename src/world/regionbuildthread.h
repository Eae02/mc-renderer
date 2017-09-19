#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "../rendering/regions/regionuploader.h"

namespace MCR
{
	class Region;
	
	class RegionBuildThread final
	{
	public:
		struct BuildCommand
		{
			RegionCoordinate m_coordinate;
			std::weak_ptr<const Region> m_region;
			std::weak_ptr<const Region> m_neighbors[4];
		};
		
		RegionBuildThread();
		~RegionBuildThread();
		
		inline void BeginUpdating()
		{
			m_mutex.lock();
			m_anyCommandsEnqueued = false;
		}
		
		//Only call between BeginUpdating and EndUpdating.
		inline void BuildASync(const BuildCommand& buildCommand)
		{
			m_buildCommands.push_back(buildCommand);
			m_anyCommandsEnqueued = true;
		}
		
		//Only call between BeginUpdating and EndUpdating.
		inline void SetCameraRegion(RegionCoordinate coordinate)
		{
			m_cameraRegion = coordinate;
		}
		
		inline void EndUpdating()
		{
			if (m_anyCommandsEnqueued)
			{
				m_signal.notify_one();
			}
			m_mutex.unlock();
		}
		
		void BuildSync(const Region& region, gsl::span<const Region*> neighbors, MeshBuilder& meshBuilder);
		
		template <typename CallbackTp>
		inline void IterateCompleted(CallbackTp callback)
		{
			m_uploader.IterateCompleted(callback);
		}
		
	private:
		void ThreadTarget();
		
		std::mutex m_mutex;
		std::condition_variable m_signal;
		
		bool m_exit = false;
		
		RegionCoordinate m_cameraRegion;
		
		bool m_anyCommandsEnqueued = false;
		std::vector<BuildCommand> m_buildCommands;
		
		MeshBuilder m_meshBuilder;
		
		RegionUploader m_uploader;
		
		std::thread m_thread;
	};
}
