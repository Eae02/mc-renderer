#include <SDL2/SDL.h>
#include <iostream>

#include "game.h"
#include "utils.h"
#include "vulkan/instance.h"
#include "rendering/shaders/shadermodules.h"
#include "rendering/setlayouts.h"
#include "ui/font.h"
#include "vulkan/setlayoutsmanager.h"

#undef main

#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

constexpr int WindowWidth = 1400;
constexpr int WindowHeight = 800;

int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cerr << SDL_GetError() << "\n";
		return 1;
	}
	
	MCR::Font::InitFreetype();
	
	if (SDL_Vulkan_LoadLibrary(nullptr) == 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Could not Load Vulkan Library", SDL_GetError(), nullptr);
		return 1;
	}
	
	SDL_Window* window = SDL_CreateWindow("Minecraft Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                      WindowWidth, WindowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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
	
	MCR::LoadShaderModules();
	MCR::RegisterSetLayouts();
	
	MCR::RunGameLoop(window);
	
	MCR::DestroyShaderModules();
	MCR::DestroyDescriptorSetLayouts();
	
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
