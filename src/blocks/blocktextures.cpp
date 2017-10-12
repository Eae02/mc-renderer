#include "blocktextures.h"

namespace MCR
{
	const BlockTextureDesc blockTextures[] = 
	{
		{ "dirt" },
		{ "grass_side" },
		{ "grass_top", true },
		{ "stone" },
		{ "bedrock" },
		{ "fern", true },
		{ "flower_blue_orchid" },
		{ "flower_rose" },
		{ "flower_oxeye_daisy" },
		{ "flower_tulip_orange" },
		{ "flower_dandelion" },
		{ "coal_ore" },
		{ "iron_ore" },
		{ "gold_ore" },
		{ "diamond_ore" },
		{ "gravel" },
		{ "sand" },
		{ "stone_andesite" },
		{ "stone_diorite" },
		{ "stone_granite" },
		{ "log_oak" },
		{ "log_oak_top" },
		{ "log_spruce" },
		{ "log_spruce_top" },
		{ "leaves_oak", true },
		{ "leaves_spruce", true }
	};
	
	const gsl::span<const BlockTextureDesc> GetBlockTexturesList()
	{
		return blockTextures;
	}
}
