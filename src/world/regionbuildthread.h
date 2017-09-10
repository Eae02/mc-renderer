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
			const Region* m_region;
			const Region* m_neighbors[4];
			uint32_t* m_numIndicesOut;
			uint32_t* m_numVerticesOut;
			gsl::span<uint32_t> m_sliceOffsetsOut;
		};
		
		RegionBuildThread();
		~RegionBuildThread();
		
		void BuildASync(const BuildCommand& buildCommand);
		
		void BuildSync(const BuildCommand& buildCommand, MeshBuilder& meshBuilder);
		
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
		
		std::queue<BuildCommand> m_buildCommands;
		
		MeshBuilder m_meshBuilder;
		
		RegionUploader m_uploader;
		
		std::thread m_thread;
	};
}
