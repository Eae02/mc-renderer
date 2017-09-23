#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <cstdint>

#include "../rendering/regions/chunkuploader.h"
#include "../rendering/regions/meshbuilder.h"

namespace MCR
{
	class ChunkBuildThread final
	{
	public:
		struct BuildCommand
		{
			RegionCoordinate m_coordinate;
			uint32_t m_chunkY;
			std::weak_ptr<const Region> m_region;
			std::weak_ptr<const Region> m_neighbors[4];
		};
		
		inline ChunkBuildThread()
		    : m_thread(&ChunkBuildThread::ThreadTarget, this) { }
		
		~ChunkBuildThread();
		
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
		inline void SetCameraPosition(int64_t chunkX, int64_t chunkY, int64_t chunkZ)
		{
			m_cameraChunkX = chunkX;
			m_cameraChunkY = chunkY;
			m_cameraChunkZ = chunkZ;
		}
		
		inline void EndUpdating()
		{
			if (m_anyCommandsEnqueued)
			{
				m_signal.notify_one();
			}
			m_mutex.unlock();
		}
		
		void BuildSync(const Region& region, uint32_t chunkY, gsl::span<const Region*> neighbors, MeshBuilder& meshBuilder);
		
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
		
		int64_t m_cameraChunkX;
		int64_t m_cameraChunkY;
		int64_t m_cameraChunkZ;
		
		bool m_anyCommandsEnqueued = false;
		std::vector<BuildCommand> m_buildCommands;
		
		MeshBuilder m_meshBuilder;
		
		ChunkUploader m_uploader;
		
		std::thread m_thread;
	};
}
