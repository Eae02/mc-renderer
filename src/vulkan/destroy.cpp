#include "destroy.h"
#include "vkutils.h"

namespace MCR
{
	struct DestroyListEntry
	{
		void* m_object;
		void (*m_destroyCallback)(void*);
		uint64_t m_enqueuedFrameIndex;
	};
	
	std::vector<DestroyListEntry> destroyList;
	
	void Detail::AddToDestroyList(void (*destroyCallback)(void*), void* object)
	{
		destroyList.push_back({ object, destroyCallback, frameIndex });
	}
	
	void ProcessVulkanDestroyList()
	{
		if (frameIndex < MaxQueuedFrames)
			return;
		frameIndex -= MaxQueuedFrames;
		
		for (size_t i = 0; i < destroyList.size();)
		{
			if (destroyList[i].m_enqueuedFrameIndex > frameIndex)
			{
				i++;
				continue;
			}
			
			destroyList[i].m_destroyCallback(destroyList[i].m_object);
			
			destroyList[i] = destroyList.back();
			destroyList.pop_back();
		}
	}
	
	void ClearVulkanDestroyList()
	{
		for (const DestroyListEntry& entry : destroyList)
		{
			entry.m_destroyCallback(entry.m_object);
		}
		
		destroyList.clear();
	}
}
