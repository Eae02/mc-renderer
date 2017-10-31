#pragma once

namespace MCR
{
namespace Arguments
{
	extern bool noValidation;
	extern bool noBackgroundTransfer;
	extern bool noVkExtensions;
	
	void Parse(int argc, char** argv);
}
}
