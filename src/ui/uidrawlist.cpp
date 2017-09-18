#include "uidrawlist.h"
#include "font.h"

#include <utf8.h>
#include <cctype>

namespace MCR
{
	glm::vec2 UIDrawList::AddText(const Font& font, std::string_view text, const glm::vec2 position,
	                              const glm::vec4 color, const TextPosX posX, const TextPosY posY)
	{
		BeginBatch(font.GetDescriptorSet());
		
		float offsetY = 0;
		float maxLineWidth = 0;
		
		if (posY != TextPosY::Below)
		{
			const size_t lineCount = std::count(MAKE_RANGE(text), '\n');
			
			if (posY == TextPosY::Center)
			{
				offsetY -= lineCount / 2.0f;
			}
			else //posY == TextPosY::Above
			{
				offsetY -= lineCount;
			}
		}
		
		//Iterates through the text line by line
		auto lineBeg = text.begin();
		while (lineBeg != text.end())
		{
			auto lineEnd = std::find(lineBeg, text.end(), '\n');
			
			float linePosX = position.x;
			
			const float lineWidth = font.GetTextExtents(std::string_view(&*lineBeg, lineEnd - lineBeg)).x;
			if (lineWidth > maxLineWidth)
			{
				maxLineWidth = lineWidth;
			}
			
			if (posX == TextPosX::Center)
			{
				linePosX -= lineWidth / 2.0f;
			}
			else if (posX == TextPosX::Left)
			{
				linePosX -= lineWidth;
			}
			
			while (lineBeg != lineEnd)
			{
				uint32_t charCode = utf8::next(lineBeg, text.end());
				
				if (charCode == ' ')
				{
					linePosX += font.GetSpaceAdvance();
					continue;
				}
				
				if (::isspace(charCode))
					continue;
				
				const Font::Glyph& glyph = font.FindGlyph(charCode);
				
				std::array<Vertex, 4> vertices;
				
				const glm::vec2 basePos(linePosX + glyph.m_bearingX, position.y + offsetY - glyph.m_bearingY + font.GetSize());
				
				for (int x = 0; x < 2; x++)
				{
					for (int y = 0; y < 2; y++)
					{
						Vertex& v = vertices[x * 2 + y];
						v.m_position = basePos + glyph.m_size * glm::vec2(x, y);
						v.m_texCoord = glm::vec3(x * glyph.m_maxTexCoord.x, y * glyph.m_maxTexCoord.y, glyph.m_texLayer);
						v.m_color = color;
					}
				}
				
				AppendQuad(vertices.data());
				
				linePosX += glyph.m_advance;
			}
			
			offsetY += font.GetSize();
		}
		
		return { maxLineWidth, offsetY };
	}
	
	void UIDrawList::CopyToBuffer(void* bufferData) const
	{
		Vertex* verticesOut = reinterpret_cast<Vertex*>(bufferData);
		uint32_t* indicesOut = reinterpret_cast<uint32_t*>(verticesOut + m_vertices.size());
		
		std::copy(MAKE_RANGE(m_vertices), verticesOut);
		std::copy(MAKE_RANGE(m_indices), indicesOut);
	}
	
	void UIDrawList::Draw(CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkBuffer buffer) const
	{
		const VkDeviceSize vertexBufferOffsets[] = { 0 };
		commandBuffer.BindVertexBuffers(0, 1, &buffer, vertexBufferOffsets);
		
		commandBuffer.BindIndexBuffer(buffer, sizeof(Vertex) * m_vertices.size(), VK_INDEX_TYPE_UINT32);
		
		for (const UIDrawList::Batch& batch : m_batches)
		{
			commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1,
			                                 SingleElementSpan(batch.m_descriptorSet));
			
			commandBuffer.DrawIndexed(batch.m_numIndices, 1, batch.m_firstIndex, 0, 0);
		}
	}
	
	void UIDrawList::AppendQuad(const UIDrawList::Vertex* vertices)
	{
		const uint32_t baseIndex = gsl::narrow_cast<uint32_t>(m_vertices.size());
		
		const uint32_t indices[] = { 0, 1, 2, 1, 2, 3 };
		for (uint32_t index : indices)
		{
			m_indices.push_back(baseIndex + index);
		}
		
		m_vertices.resize(m_vertices.size() + 4);
		std::copy_n(vertices, 4, m_vertices.end() - 4);
		
		m_batches.back().m_numIndices += 6;
	}
	
	void UIDrawList::BeginBatch(VkDescriptorSet descriptorSet)
	{
		if (m_batches.empty() || m_batches.back().m_descriptorSet != descriptorSet)
		{
			m_batches.push_back({ gsl::narrow_cast<uint32_t>(m_indices.size()), 0, descriptorSet });
		}
	}
	
	
}
