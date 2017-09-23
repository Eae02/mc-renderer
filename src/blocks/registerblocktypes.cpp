#include "registerblocktypes.h"
#include "blocktype.h"
#include "ids.h"
#include "../rendering/meshes/flowermeshprovider.h"

namespace MCR
{
	void RegisterBlockTypes()
	{
		BlockType::Register(BlockIDs::Grass, "Grass", BlockTexConfig::UniformSide("grass_top.png", "grass_side.png", "dirt.png"));
		BlockType::Register(BlockIDs::Dirt, "Dirt", BlockTexConfig::Uniform("dirt.png"));
		BlockType::Register(BlockIDs::Stone, "Stone", BlockTexConfig::Uniform("stone.png"));
		BlockType::Register(BlockIDs::Bedrock, "Bedrock", BlockTexConfig::Uniform("bedrock.png"));
		
		BlockType::Register(BlockIDs::CoalOre, "Coal Ore", BlockTexConfig::Uniform("coal_ore.png"));
		BlockType::Register(BlockIDs::IronOre, "Iron Ore", BlockTexConfig::Uniform("iron_ore.png"));
		BlockType::Register(BlockIDs::GoldOre, "Gold Ore", BlockTexConfig::Uniform("gold_ore.png"));
		BlockType::Register(BlockIDs::DiamondOre, "Diamond Ore", BlockTexConfig::Uniform("diamond_ore.png"));
		
		BlockType::RegisterCustomMesh(BlockIDs::BlueFlower, "Blue Flower", std::make_unique<FlowerMeshProvider>("flower_blue.png"));
		BlockType::RegisterCustomMesh(BlockIDs::RedFlower, "Red Flower", std::make_unique<FlowerMeshProvider>("flower_red.png"));
		BlockType::RegisterCustomMesh(BlockIDs::YellowFlower, "Yellow Flower", std::make_unique<FlowerMeshProvider>("flower_yellow.png"));
		BlockType::RegisterCustomMesh(BlockIDs::OrangeFlower, "Orange Flower", std::make_unique<FlowerMeshProvider>("flower_orange.png"));
		BlockType::RegisterCustomMesh(BlockIDs::WhiteFlower, "White Flower", std::make_unique<FlowerMeshProvider>("flower_white.png"));
	}
}
