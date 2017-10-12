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
		
		BlockType::RegisterCustomMesh(BlockIDs::Fern, "Fern", std::make_unique<FlowerMeshProvider>("fern", 1.0f));
		BlockType::RegisterCustomMesh(BlockIDs::BlueFlower, "Blue Flower", std::make_unique<FlowerMeshProvider>("flower_blue_orchid", 0.8f));
		BlockType::RegisterCustomMesh(BlockIDs::RedFlower, "Red Flower", std::make_unique<FlowerMeshProvider>("flower_rose", 0.8f));
		BlockType::RegisterCustomMesh(BlockIDs::YellowFlower, "Yellow Flower", std::make_unique<FlowerMeshProvider>("flower_oxeye_daisy", 0.8f));
		BlockType::RegisterCustomMesh(BlockIDs::OrangeFlower, "Orange Flower", std::make_unique<FlowerMeshProvider>("flower_tulip_orange", 0.8f));
		BlockType::RegisterCustomMesh(BlockIDs::WhiteFlower, "White Flower", std::make_unique<FlowerMeshProvider>("flower_dandelion", 0.8f));
		
		BlockType::Register(BlockIDs::OakWood, "Oak Wood", BlockTexConfig::UniformSideTop("log_oak_top", "log_oak"), 0.7f);
		BlockType::Register(BlockIDs::OakLeaves, "Oak Leaves", BlockTexConfig::Uniform("leaves_oak"), 0.7f, false);
		
		BlockType::Register(BlockIDs::SpruceWood, "Spruce Wood", BlockTexConfig::UniformSideTop("log_spruce_top", "log_spruce"), 0.7f);
		BlockType::Register(BlockIDs::SpruceLeaves, "Spruce Leaves", BlockTexConfig::Uniform("leaves_spruce"), 0.7f, false, 0.3f);
	}
}
