#include "regionbuildthread.h"
#include "region.h"
#include "../rendering/regions/buildregionmesh.h"

namespace MCR
{
	RegionBuildThread::RegionBuildThread()
	    : m_thread(&RegionBuildThread::ThreadTarget, this)
	{
		
	}
	
	RegionBuildThread::~RegionBuildThread()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_exit = true;
			m_signal.notify_one();
		}
		
		m_thread.join();
	}
	
	void RegionBuildThread::BuildASync(const BuildCommand& buildCommand)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_buildCommands.push(buildCommand);
		m_signal.notify_one();
	}
	
	void RegionBuildThread::BuildSync(const BuildCommand& buildCommand, MeshBuilder& meshBuilder)
	{
		RegionMeshBuildParams buildParams;
		buildParams.m_meshBuilder = &meshBuilder;
		buildParams.m_region = buildCommand.m_region;
		buildParams.m_sliceOffsets = buildCommand.m_sliceOffsetsOut;
		std::copy(MAKE_RANGE(buildCommand.m_neighbors), buildParams.m_neighbors);
		
		meshBuilder.Reset();
		
		BuildRegionMesh(buildParams);
		
		*buildCommand.m_numIndicesOut = meshBuilder.GetNumIndices();
		*buildCommand.m_numVerticesOut = meshBuilder.GetNumVertices();
		
		m_uploader.BeginUploading(buildCommand.m_region->GetX(), buildCommand.m_region->GetZ(), meshBuilder);
	}
	
	void RegionBuildThread::ThreadTarget()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			
			m_signal.wait(lock, [&] { return !m_buildCommands.empty() || m_exit; });
			
			if (m_exit)
				break;
			
			BuildCommand buildCommand = m_buildCommands.front();
			m_buildCommands.pop();
			
			lock.unlock();
			
			BuildSync(buildCommand, m_meshBuilder);
		}
	}
}
