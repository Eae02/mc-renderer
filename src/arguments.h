#pragma once

namespace MCR
{
namespace Arguments
{
	extern bool noValidation;
	extern bool noBackgroundTransfer;
	
	void Parse(int argc, char** argv);
}
}
