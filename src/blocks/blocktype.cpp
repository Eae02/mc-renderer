#include "blocktype.h"
#include "blockstexturemanager.h"
#include "sides.h"
#include "../utils.h"

#include <fstream>

namespace MCR
{
	BlockType BlockType::s_blockTypes[256];
	
	BlockType::BlockType()
	    : m_initialized(false), m_opaque(true), m_roughness(1.0f)
	{
		std::fill(MAKE_RANGE(m_albedoTextureIndices), -1);
	}
	
	void BlockType::ThrowIfInitialized(uint8_t id)
	{
		if (s_blockTypes[id].m_initialized)
		{
			throw std::runtime_error("Block id " + std::to_string(id) + " is already used.");
		}
	}
	
	void BlockType::Register(uint8_t id, std::string name, const BlockTexConfig& texConfig, float roughness,
	                         bool opaque, float bendiness)
	{
		ThrowIfInitialized(id);
		
		BlockType& blockType = s_blockTypes[id];
		
		blockType.m_initialized = true;
		blockType.m_opaque = opaque;
		blockType.m_roughness = roughness;
		blockType.m_bendiness = bendiness;
		
		blockType.m_name = std::move(name);
		
		for (int i = 0; i < 6; i++)
		{
			BlocksTextureManager::GetInstance().GetTextureIndices(texConfig.m_textures[i],
			                                                      blockType.m_albedoTextureIndices[i],
			                                                      blockType.m_normalTextureIndices[i]);
		}
	}
	
	void BlockType::RegisterCustomMesh(uint8_t id, std::string name, std::unique_ptr<ICustomMeshProvider> meshProvider)
	{
		ThrowIfInitialized(id);
		
		BlockType& blockType = s_blockTypes[id];
		
		blockType.m_name = std::move(name);
		blockType.m_customMeshProvider = std::move(meshProvider);
		blockType.m_initialized = true;
		blockType.m_opaque = false;
	}
}
