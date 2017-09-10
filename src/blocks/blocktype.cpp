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
	
	void BlockType::LoadBlockTypesFromDirectory(const fs::path& dirPath, const BlocksTextureManager& textureManager)
	{
		for (const fs::directory_entry& entry : fs::recursive_directory_iterator(dirPath))
		{
			if (entry.status().type() != fs::file_type::regular || entry.path().extension() != ".json")
				continue;
			
			std::ifstream stream(entry.path());
			nlohmann::json json = nlohmann::json::parse(stream);
			LoadBlockType(json, textureManager);
		}
	}
	
	void BlockType::LoadBlockType(const nlohmann::json& json, const BlocksTextureManager& textureManager)
	{
		BlockType blockType;
		
		uint32_t id = json.at("id");
		if (s_blockTypes[id].m_initialized)
		{
			throw std::runtime_error("Block id " + std::to_string(id) + " is already used.");
		}
		
		blockType.m_name = json.at("name").get<std::string>();
		blockType.m_initialized = true;
		
		auto textureIt = json.find("texture");
		if (textureIt->is_string())
		{
			const int textureIndex = textureManager.FindTextureIndex(textureIt->get<std::string>());
			std::fill(MAKE_RANGE(blockType.m_textureIndices), textureIndex);
		}
		else if (textureIt->is_object())
		{
			auto GetTextureIndex = [&] (const std::string& elementName, int def = -1)
			{
				auto it = textureIt->find(elementName);
				if (it != textureIt->end() && it->is_string())
				{
					return textureManager.FindTextureIndex(it->get<std::string>());
				}
				return def;
			};
			
			int sideTextureIndex = GetTextureIndex("side");
			
			std::fill(MAKE_RANGE(blockType.m_textureIndices), sideTextureIndex);
			
			blockType.m_textureIndices[BLOCK_SIDE_POSY] = GetTextureIndex("top", sideTextureIndex);
			blockType.m_textureIndices[BLOCK_SIDE_NEGY] = GetTextureIndex("bottom", sideTextureIndex);
		}
		
		s_blockTypes[id] = std::move(blockType);
	}
}
