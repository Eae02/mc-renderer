#include "registerblocktypes.h"
#include "blocktype.h"
#include "ids.h"
#include "../rendering/meshes/flowermeshprovider.h"

namespace MCR
{
	void RegisterBlockTypes()
	{
		BlockType::Register(BlockIDs::Grass, "Grass", BlockTexConfig::UniformSide("grass_top", "grass_side", "dirt"), 0.8f);
		BlockType::Register(BlockIDs::Dirt, "Dirt", BlockTexConfig::Uniform("dirt"), 0.8f);
		BlockType::Register(BlockIDs::Stone, "Stone", BlockTexConfig::Uniform("stone"), 0.5f);
		BlockType::Register(BlockIDs::Bedrock, "Bedrock", BlockTexConfig::Uniform("bedrock"), 0.5f);
		
		BlockType::Register(BlockIDs::CoalOre, "Coal Ore", BlockTexConfig::Uniform("coal_ore"), 0.5f);
		BlockType::Register(BlockIDs::IronOre, "Iron Ore", BlockTexConfig::Uniform("iron_ore"), 0.5f);
		BlockType::Register(BlockIDs::GoldOre, "Gold Ore", BlockTexConfig::Uniform("gold_ore"), 0.5f);
		BlockType::Register(BlockIDs::DiamondOre, "Diamond Ore", BlockTexConfig::Uniform("diamond_ore"), 0.5f);
		
		BlockType::RegisterCustomMesh(BlockIDs::BlueFlower, "Blue Flower", std::make_unique<FlowerMeshProvider>("flower_blue_orchid"));
		BlockType::RegisterCustomMesh(BlockIDs::RedFlower, "Red Flower", std::make_unique<FlowerMeshProvider>("flower_rose"));
		BlockType::RegisterCustomMesh(BlockIDs::YellowFlower, "Yellow Flower", std::make_unique<FlowerMeshProvider>("flower_oxeye_daisy"));
		BlockType::RegisterCustomMesh(BlockIDs::OrangeFlower, "Orange Flower", std::make_unique<FlowerMeshProvider>("flower_tulip_orange"));
		BlockType::RegisterCustomMesh(BlockIDs::WhiteFlower, "White Flower", std::make_unique<FlowerMeshProvider>("flower_dandelion"));
	}
}
