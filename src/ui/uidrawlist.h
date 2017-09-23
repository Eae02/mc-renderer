#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../vulkan/vk.h"

namespace MCR
{
	enum class TextPosY
	{
		Above,
		Below,
		Center
	};
	
	enum class TextPosX
	{
		Left,
		Right,
		Center
	};
	
	class UIDrawList
	{
		friend class UIGraphicsContext;
		
	public:
#pragma pack(push, 1)
		struct Vertex
		{
			glm::vec2 m_position;
			glm::vec3 m_texCoord;
			glm::vec4 m_color;
		};
#pragma pack(pop)
		
		UIDrawList() = default;
		
		inline void Reset()
		{
			m_batches.clear();
			m_vertices.clear();
			m_indices.clear();
		}
		
		glm::vec2 AddText(const class Font& font, std::string_view text, glm::vec2 position, const glm::vec4& color,
		                  TextPosX posX = TextPosX::Right, TextPosY posY = TextPosY::Below);
		
		void AddRectangle(glm::vec2 pos1, glm::vec2 pos2, const glm::vec4& color);
		
		void AddLine(glm::vec2 pos1, glm::vec2 pos2, const glm::vec4& color, float width = 1.0f);
		
		void AddTriangle(const glm::vec2* positions, const glm::vec4& color);
		void AddTriangleMultiColored(const glm::vec2* positions, const glm::vec4* colors);
		
		void AddList(const UIDrawList& other);
		
		inline bool Empty() const
		{
			return m_batches.empty();
		}
		
	private:
		inline uint64_t GetRequiredBufferSize() const
		{
			return m_vertices.size() * sizeof(Vertex) + m_indices.size() * sizeof(uint32_t);
		}
		
		void CopyToBuffer(void* bufferData) const;
		
		void Draw(CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet defaultDS, VkBuffer buffer) const;
		
		struct Batch
		{
			uint32_t m_firstIndex;
			uint32_t m_numIndices;
			VkDescriptorSet m_descriptorSet;
		};
		
		void AppendTriangle(const Vertex* vertices);
		void AppendQuad(const Vertex* vertices);
		
		void BeginBatch(VkDescriptorSet descriptorSet);
		
		std::vector<Batch> m_batches;
		
		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};
}
