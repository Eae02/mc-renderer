#include "blocktype.h"
#include "blockstexturemanager.h"
#include "sides.h"
#include "../utils.h"

#include <fstream>
#include <algorithm>

namespace MCR
{
	BlockType BlockType::s_blockTypes[256];
	
	static void ParseTextures(const nlohmann::json& json, int* albedoTextureIndices, int* normalTextureIndices)
	{
		BlocksTextureManager& blocksTextureManager = BlocksTextureManager::GetInstance();
		
		auto allIt = json.find("all");
		if (allIt != json.end())
		{
			std::string_view name = *allIt->get<const std::string*>();
			blocksTextureManager.GetTextureIndices(name, albedoTextureIndices[0], normalTextureIndices[0]);
			
			std::fill_n(albedoTextureIndices + 1, 5, albedoTextureIndices[0]);
			std::fill_n(normalTextureIndices + 1, 5, normalTextureIndices[0]);
			
			return;
		}
		
		const char dimensionChars[] = { 'x', 'y', 'z' };
		
		for (int i = 0; i < 3; i++)
		{
			int* dimAlbedoIndices = albedoTextureIndices + i * 2;
			int* dimNormalIndices = normalTextureIndices + i * 2;
			
			const char uniformName[] = { dimensionChars[i], '\0' };
			auto uniformIt = json.find(uniformName);
			if (uniformIt != json.end())
			{
				std::string_view name = *uniformIt->get<const std::string*>();
				blocksTextureManager.GetTextureIndices(name, dimAlbedoIndices[0], dimNormalIndices[0]);
				dimAlbedoIndices[1] = dimAlbedoIndices[0];
				dimNormalIndices[1] = dimNormalIndices[0];
			}
			else
			{
				const char negName[] = { '-', dimensionChars[i], '\0' };
				auto negNameIt = json.find(negName);
				if (negNameIt != json.end())
				{
					std::string_view name = *negNameIt->get<const std::string*>();
					blocksTextureManager.GetTextureIndices(name, dimAlbedoIndices[1], dimNormalIndices[1]);
				}
				else
				{
					dimAlbedoIndices[1] = 0;
					dimNormalIndices[1] = -1;
				}
				
				const char posName[] = { '+', dimensionChars[i], '\0' };
				auto posNameIt = json.find(posName);
				if (posNameIt != json.end())
				{
					std::string_view name = *posNameIt->get<const std::string*>();
					blocksTextureManager.GetTextureIndices(name, dimAlbedoIndices[0], dimNormalIndices[0]);
				}
				else
				{
					dimAlbedoIndices[0] = 0;
					dimNormalIndices[0] = -1;
				}
			}
		}
	}
	
	void BlockType::Parse(const nlohmann::json& json)
	{
		m_initialized = true;
		m_name = json.at("name").get<std::string>();
		
		auto opaqueIt = json.find("opaque");
		if (opaqueIt != json.end())
		{
			m_opaque = opaqueIt->get<bool>();
		}
		
		auto bendinessIt = json.find("bendiness");
		if (bendinessIt != json.end())
		{
			m_bendiness = glm::max(bendinessIt->get<float>(), 0.0f);
		}
		
		auto roughnessIt = json.find("roughness");
		if (roughnessIt != json.end())
		{
			m_roughness = glm::clamp(roughnessIt->get<float>(), 0.0f, 1.0f);
		}
		
		//Parses textures
		auto textureIt = json.find("texture");
		if (textureIt != json.end())
		{
			ParseTextures(*textureIt, m_albedoTextureIndices, m_normalTextureIndices);
		}
	}
	
	void BlockType::RegisterJSON(const nlohmann::json& json)
	{
		uint8_t id = json.at("id").get<uint8_t>();
		if (s_blockTypes[id].m_initialized)
		{
			throw std::runtime_error("Multiple blocks with id: " + std::to_string(id) + ".");
		}
		
		s_blockTypes[id].Parse(json);
	}
	
	void BlockType::SetCustomMesh(uint8_t id, std::unique_ptr<ICustomMeshProvider> meshProvider)
	{
		BlockType& blockType = s_blockTypes[id];
		if (!blockType.m_initialized)
		{
			throw std::runtime_error("Block type not initialized.");
		}
		
		blockType.m_customMeshProvider = std::move(meshProvider);
		blockType.m_opaque = false;
	}
}
