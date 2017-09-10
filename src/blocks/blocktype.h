#pragma once

#include <string>
#include <json.hpp>

#include "../filesystem.h"

namespace MCR
{
	class BlockType
	{
	public:
		BlockType();
		
		static void LoadBlockTypesFromDirectory(const fs::path& dirPath,
		                                        const class BlocksTextureManager& textureManager);
		
		static void LoadBlockType(const nlohmann::json& json, const class BlocksTextureManager& textureManager);
		
		inline static BlockType& GetByID(uint8_t id)
		{
			return s_blockTypes[id];
		}
		
		inline bool IsInitialized() const
		{
			return m_initialized;
		}
		
		inline bool IsOpaque() const
		{
			return m_initialized && m_opaque;
		}
		
		inline const std::string& GetName() const
		{
			return m_name;
		}
		
		inline int GetTextureLayer(int side) const
		{
			return m_textureIndices[side];
		}
		
	private:
		static BlockType s_blockTypes[256];
		
		bool m_initialized;
		std::string m_name;
		int m_textureIndices[6];
		bool m_opaque;
	};
}
