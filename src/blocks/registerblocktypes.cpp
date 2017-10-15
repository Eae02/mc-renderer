#include "registerblocktypes.h"
#include "blocktype.h"
#include "setcustommeshes.h"
#include "../filesystem.h"

#include <fstream>

namespace MCR
{
	void RegisterBlockTypes()
	{
		for (fs::directory_entry dirEntry : fs::recursive_directory_iterator(GetResourcePath() / "blocks"))
		{
			if (dirEntry.status().type() != fs::file_type::regular)
				continue;
			if (dirEntry.path().extension() != ".json")
				continue;
			
			std::ifstream stream(dirEntry.path());
			if (!stream)
			{
				throw std::runtime_error("Error opening block definition file for reading '" +
				                         dirEntry.path().string() + "'.");
			}
			
			nlohmann::json json = nlohmann::json::parse(stream);
			
			stream.close();
			
			BlockType::RegisterJSON(json);
		}
		
		SetCustomBlockMeshes();
	}
}
