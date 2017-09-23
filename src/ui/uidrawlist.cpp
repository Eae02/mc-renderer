#include "uidrawlist.h"
#include "font.h"

#include <utf8.h>
#include <cctype>

namespace MCR
{
	glm::vec2 UIDrawList::AddText(const Font& font, std::string_view text, const glm::vec2 position,
	                              const glm::vec4& color, const TextPosX posX, const TextPosY posY)
	{
		BeginBatch(font.GetDescriptorSet());
		
		float offsetY = 0;
		float maxLineWidth = 0;
		
		if (posY != TextPosY::Below)
		{
			const size_t lineCount = std::count(MAKE_RANGE(text), '\n') + 1;
			
			if (posY == TextPosY::Center)
			{
				offsetY -= (lineCount * font.GetSize()) / 2.0f;
			}
			else //posY == TextPosY::Above
			{
				offsetY -= (lineCount * font.GetSize());
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
	
	void UIDrawList::AddRectangle(const glm::vec2 pos1, const glm::vec2 pos2, const glm::vec4& color)
	{
		BeginBatch(VK_NULL_HANDLE);
		
		std::array<Vertex, 4> vertices;
		
		glm::vec2 d = pos2 - pos1;
		
		for (int x = 0; x < 2; x++)
		{
			for (int y = 0; y < 2; y++)
			{
				Vertex& v = vertices[x * 2 + y];
				v.m_position = pos1 + d * glm::vec2(x, y);
				v.m_texCoord = glm::vec3();
				v.m_color = color;
			}
		}
		
		AppendQuad(vertices.data());
	}
	
	void UIDrawList::AddLine(glm::vec2 pos1, glm::vec2 pos2, const glm::vec4& color, float width)
	{
		BeginBatch(VK_NULL_HANDLE);
		
		std::array<Vertex, 4> vertices;
		
		glm::vec2 toEdgeF = glm::normalize(pos2 - pos1) * width * 0.5f;
		glm::vec2 toEdgeL(-toEdgeF.y, toEdgeF.x);
		
		vertices[0].m_position = pos1 - toEdgeF - toEdgeL;
		vertices[1].m_position = pos1 - toEdgeF + toEdgeL;
		vertices[2].m_position = pos2 + toEdgeF - toEdgeL;
		vertices[3].m_position = pos2 + toEdgeF + toEdgeL;
		
		for (int i = 0; i < 4; i++)
		{
			vertices[i].m_color = color;
			vertices[i].m_texCoord = glm::vec3();
		}
		
		AppendQuad(vertices.data());
	}
	
	void UIDrawList::AddTriangle(const glm::vec2* positions, const glm::vec4& color)
	{
		BeginBatch(VK_NULL_HANDLE);
		
		std::array<Vertex, 3> vertices;
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i].m_position = positions[i];
			vertices[i].m_texCoord = glm::vec3();
			vertices[i].m_color = color;
		}
		
		AppendTriangle(vertices.data());
	}
	
	void UIDrawList::AddTriangleMultiColored(const glm::vec2* positions, const glm::vec4* colors)
	{
		BeginBatch(VK_NULL_HANDLE);
		
		std::array<Vertex, 3> vertices;
		for (size_t i = 0; i < vertices.size(); i++)
		{
			vertices[i].m_position = positions[i];
			vertices[i].m_texCoord = glm::vec3();
			vertices[i].m_color = colors[i];
		}
		
		AppendTriangle(vertices.data());
	}
	
	void UIDrawList::AddList(const UIDrawList& other)
	{
		if (other.m_batches.empty())
			return;
		
		const uint32_t indexOffset = static_cast<uint32_t>(m_vertices.size());
		m_vertices.insert(m_vertices.end(), MAKE_RANGE(other.m_vertices));
		
		for (const Batch& batch : other.m_batches)
		{
			BeginBatch(batch.m_descriptorSet);
			
			m_batches.back().m_numIndices = batch.m_numIndices;
			
			const uint32_t* srcIndices = other.m_indices.data() + batch.m_firstIndex;
			
			m_indices.resize(m_indices.size() + batch.m_numIndices);
			for (uint32_t i = 1; i <= batch.m_numIndices; i++)
			{
				m_indices[m_indices.size() - i] = srcIndices[batch.m_numIndices - i] + indexOffset;
			}
		}
	}
	
	void UIDrawList::CopyToBuffer(void* bufferData) const
	{
		Vertex* verticesOut = reinterpret_cast<Vertex*>(bufferData);
		uint32_t* indicesOut = reinterpret_cast<uint32_t*>(verticesOut + m_vertices.size());
		
		std::copy(MAKE_RANGE(m_vertices), verticesOut);
		std::copy(MAKE_RANGE(m_indices), indicesOut);
	}
	
	void UIDrawList::Draw(CommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet defaultDS,
	                      VkBuffer buffer) const
	{
		const VkDeviceSize vertexBufferOffsets[] = { 0 };
		commandBuffer.BindVertexBuffers(0, 1, &buffer, vertexBufferOffsets);
		
		commandBuffer.BindIndexBuffer(buffer, sizeof(Vertex) * m_vertices.size(), VK_INDEX_TYPE_UINT32);
		
		for (const UIDrawList::Batch& batch : m_batches)
		{
			const VkDescriptorSet ds = batch.m_descriptorSet ? batch.m_descriptorSet : defaultDS;
			
			commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, SingleElementSpan(ds));
			
			commandBuffer.DrawIndexed(batch.m_numIndices, 1, batch.m_firstIndex, 0, 0);
		}
	}
	
	void UIDrawList::AppendTriangle(const UIDrawList::Vertex* vertices)
	{
		const uint32_t baseIndex = static_cast<uint32_t>(m_vertices.size());
		
		for (uint32_t i = 0; i < 3; i++)
		{
			m_indices.push_back(baseIndex + i);
		}
		
		m_vertices.resize(m_vertices.size() + 3);
		std::copy_n(vertices, 3, m_vertices.end() - 3);
		
		m_batches.back().m_numIndices += 3;
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
