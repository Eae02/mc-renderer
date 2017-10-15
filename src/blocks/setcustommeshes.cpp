#include "setcustommeshes.h"
#include "blocktype.h"
#include "ids.h"
#include "../rendering/meshes/flowermeshprovider.h"

namespace MCR
{
	void SetCustomBlockMeshes()
	{
		BlockType::SetCustomMesh(BlockIDs::Fern, std::make_unique<FlowerMeshProvider>("fern", 1.0f));
		BlockType::SetCustomMesh(BlockIDs::BlueFlower, std::make_unique<FlowerMeshProvider>("flower_blue_orchid", 0.8f));
		BlockType::SetCustomMesh(BlockIDs::RedFlower, std::make_unique<FlowerMeshProvider>("flower_rose", 0.8f));
		BlockType::SetCustomMesh(BlockIDs::YellowFlower, std::make_unique<FlowerMeshProvider>("flower_oxeye_daisy", 0.8f));
		BlockType::SetCustomMesh(BlockIDs::OrangeFlower, std::make_unique<FlowerMeshProvider>("flower_tulip_orange", 0.8f));
		BlockType::SetCustomMesh(BlockIDs::WhiteFlower, std::make_unique<FlowerMeshProvider>("flower_dandelion", 0.8f));
	}
}
