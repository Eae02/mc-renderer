#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>
#include <iostream>
#include <gsl/gsl_util>

#include "game.h"
#include "vulkan/instance.h"

#undef main

constexpr int WindowWidth = 1400;
constexpr int WindowHeight = 800;

int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cerr << SDL_GetError() << "\n";
		return 1;
	}
	
	auto _0 = gsl::finally(&SDL_Quit);
	
	if (IMG_Init(IMG_INIT_PNG) == 0)
	{
		std::cerr << IMG_GetError() << "\n";
		return 1;
	}
	auto _2 = gsl::finally(&IMG_Quit);
	
	SDL_Window* window = SDL_CreateWindow("Minecraft Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                      WindowWidth, WindowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error Creating window", SDL_GetError(), nullptr);
		return 1;
	}
	
	MCR::InitializeVulkan(window);
	
	MCR::RunGameLoop(window);
	
	MCR::DestroyVulkan();
	
	SDL_DestroyWindow(window);
}
