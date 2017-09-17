#include "blocktype.h"
#include "blockstexturemanager.h"
#include "sides.h"
#include "../utils.h"

#include <fstream>

namespace MCR
{
	BlockType BlockType::s_blockTypes[256];
	
	BlockType::BlockType()
	    : m_initialized(false), m_opaque(true)
	{
		std::fill(MAKE_RANGE(m_textureIndices), -1);
	}
	
	void BlockType::ThrowIfInitialized(uint8_t id)
	{
		if (s_blockTypes[id].m_initialized)
		{
			throw std::runtime_error("Block id " + std::to_string(id) + " is already used.");
		}
	}
	
	void BlockType::Register(uint8_t id, std::string name, const BlockTexConfig& texConfig)
	{
		ThrowIfInitialized(id);
		
		BlockType& blockType = s_blockTypes[id];
		
		blockType.m_initialized = true;
		
		blockType.m_name = std::move(name);
		
		std::transform(MAKE_RANGE(texConfig.m_textures), blockType.m_textureIndices, [] (std::string_view texName)
		{
			return BlocksTextureManager::GetInstance().FindTextureIndex(texName);
		});
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
