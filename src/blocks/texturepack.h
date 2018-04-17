#pragma once

#include "../filesystem.h"

#include <zip.h>
#include <glm/glm.hpp>
#include <memory>

namespace MCR
{
	class TexturePack
	{
		struct TextureDataDeleter
		{
			inline void operator()(void* data) const
			{
				std::free(data);
			}
		};
		
	public:
		class Texture
		{
			friend class TexturePack;
			
		public:
			Texture() = default;
			
			inline const uint8_t* GetData() const
			{ return m_data.get(); }
			
			void MultiplyColor(const glm::vec4& color);
			
			inline int GetWidth() const
			{ return m_width; }
			inline int GetHeight() const
			{ return m_height; }
			
		private:
			inline Texture(uint8_t* data, int width, int height)
			    : m_data(data), m_width(width), m_height(height) { }
			
			std::unique_ptr<uint8_t, TextureDataDeleter> m_data;
			int m_width;
			int m_height;
		};
		
		explicit TexturePack(const fs::path& path);
		
		Texture LoadTexture(const std::string& path, int width, int height) const;
		
	private:
		struct ZipDeleter
		{
			inline void operator()(zip* archive) const
			{
				zip_close(archive);
			}
		};
		
		std::unique_ptr<zip, ZipDeleter> m_archive;
	};
}
