#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <stdexcept>
#include <iostream>

#include "game.h"
#include "settings.h"
#include "utils.h"
#include "arguments.h"
#include "vulkan/instance.h"
#include "rendering/shaders/shadermodules.h"
#include "rendering/setlayouts.h"
#include "ui/font.h"
#include "vulkan/library.h"

#undef main

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main(int argc, char** argv)
{
	MCR::Arguments::Parse(argc, argv);
	
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cerr << SDL_GetError() << "\n";
		return 1;
	}
	
	if (!MCR::LoadVulkanLibrary())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Loading Vulkan", "Could not load vulkan library, make "
			"sure you have a working vulkan driver installed.", nullptr);
		return 2;
	}
	
	MCR::Settings::QueryAvailableDisplayModes();
	
	MCR::Font::InitFreetype();
	
	SDL_DisplayMode currentDisplayMode;
	SDL_GetCurrentDisplayMode(0, &currentDisplayMode);
	int windowWidth = static_cast<int>(currentDisplayMode.w * 0.8);
	int windowHeight = static_cast<int>(currentDisplayMode.h * 0.8);
	
	SDL_Window* window = SDL_CreateWindow("Minecraft Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                      windowWidth, windowHeight, SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Creating Window", SDL_GetError(), nullptr);
		return 2;
	}
	
	MCR::SetThreadDesc(std::this_thread::get_id(), "Main");
	
	try
	{
		MCR::InitializeVulkan(window);
	}
	catch (const std::exception& ex)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Initializing Vulkan", ex.what(), nullptr);
		return 3;
	}
	
#ifndef MCR_DEBUG
	try
	{
#endif
	
	MCR::CreateSamplers();
	MCR::LoadShaderModules();
	MCR::RegisterSetLayouts();
	
	MCR::RunGameLoop(window);
	
	MCR::DestroyShaderModules();
	MCR::DestroyDescriptorSetLayouts();
	MCR::DestroySamplers();
	
	MCR::DestroyVulkan();
	
#ifndef MCR_DEBUG
	}
	catch (const std::exception& ex)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Unexpected Runtime Error", ex.what(), nullptr);
		return -1;
	}
#endif
	
	SDL_DestroyWindow(window);
	
	MCR::Font::DestroyFreeType();
	
	SDL_Quit();
}
