#include "texturepack.h"

#include <stb_image.h>

namespace MCR
{
	inline zip* OpenArchive(const fs::path& path)
	{
		std::string pathString = path.string();
		zip* archive = zip_open(pathString.c_str(), ZIP_RDONLY, nullptr);
		
		if (archive == nullptr)
			throw std::runtime_error("Error opening texture pack.");
		
		return archive;
	}
	
	TexturePack::TexturePack(const fs::path& path)
	    : m_archive(OpenArchive(path))
	{
		
	}
	
	TexturePack::Texture TexturePack::LoadTexture(const std::string& path) const
	{
		int64_t fileIndex = zip_name_locate(m_archive.get(), path.c_str(), 0);
		if (fileIndex == -1)
			return { };
		
		zip_stat_t stat;
		zip_stat_index(m_archive.get(), fileIndex, 0, &stat);
		
		std::vector<uint8_t> data(stat.size);
		
		zip_file_t* file = zip_fopen_index(m_archive.get(), fileIndex, 0);
		zip_fread(file, data.data(), data.size());
		zip_fclose(file);
		
		int width, height;
		stbi_uc* imageData = stbi_load_from_memory(data.data(), data.size(), &width, &height, nullptr, 4);
		if (imageData == nullptr)
			return { };
		
		return { imageData, width, height };
	}
	
	void TexturePack::Texture::MultiplyColor(const glm::vec4& color)
	{
		uint8_t* end = m_data.get() + (m_width * m_height * 4);
		
		for (uint8_t* pixel = m_data.get(); pixel != end; pixel += 4)
		{
			for (int i = 0; i < 4; i++)
			{
				float valueF = static_cast<float>(pixel[i]) / 255.0f;
				valueF *= color[i];
				pixel[i] = valueF * 255.0f;
			}
		}
	}
}
