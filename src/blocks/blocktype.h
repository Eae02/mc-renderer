#pragma once

#include <string>
#include <json.hpp>

#include "sides.h"
#include "blocktexconfig.h"
#include "../filesystem.h"
#include "../rendering/meshes/icustommeshprovider.h"

namespace MCR
{
	class BlockType
	{
	public:
		BlockType() = default;
		
		static void RegisterJSON(const nlohmann::json& json);
		
		static void SetCustomMesh(uint8_t id, std::unique_ptr<ICustomMeshProvider> meshProvider);
		
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
		
		inline int GetAlbedoTextureLayer(int side) const
		{
			return m_albedoTextureIndices[side];
		}
		
		inline int GetNormalTextureLayer(int side) const
		{
			return m_normalTextureIndices[side];
		}
		
		inline float GetRoughness() const
		{
			return m_roughness;
		}
		
		inline float GetBendiness() const
		{
			return m_bendiness;
		}
		
		inline const ICustomMeshProvider* GetCustomMeshProvider() const
		{
			return m_customMeshProvider.get();
		}
		
		inline void SetCustomMeshProvider(std::unique_ptr<ICustomMeshProvider> meshProvider)
		{
			m_customMeshProvider = std::move(meshProvider);
		}
		
	private:
		void Parse(const nlohmann::json& json);
		
		static void ThrowIfInitialized(uint8_t id);
		
		static BlockType s_blockTypes[256];
		
		std::unique_ptr<ICustomMeshProvider> m_customMeshProvider;
		
		bool m_initialized = false;
		std::string m_name;
		
		int m_albedoTextureIndices[6];
		int m_normalTextureIndices[6];
		bool m_opaque = true;
		float m_bendiness = 0.0f;
		float m_roughness = 1.0f;
	};
}
