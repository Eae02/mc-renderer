#include "game.h"
#include "vulkan/vkutils.h"
#include "rendering/renderer.h"
#include "rendering/rendererframebuffer.h"

#include <SDL2/SDL_vulkan.h>
#include <cstdint>
#include <chrono>

namespace MCR
{
	void UpdateGame(float dt)
	{
		
	}
	
	void RunGameLoop(SDL_Window* window)
	{
		Renderer renderer;
		RendererFramebuffer framebuffer;
		
		VkHandle<VkFence> fences[MaxQueuedFrames];
		VkHandle<VkSemaphore> signalSemaphores[MaxQueuedFrames];
		
		for (size_t i = 0; i < MaxQueuedFrames; i++)
		{
			signalSemaphores[i] = CreateVkSemaphore();
		}
		
		std::chrono::nanoseconds lastFrameTime;
		
		int currentDrawableWidth = 0;
		int currentDrawableHeight = 0;
		
		while (true)
		{
			auto frameBeginTime = std::chrono::high_resolution_clock::now();
			
			bool shouldQuit = false;
			
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if (event.type == SDL_QUIT)
				{
					shouldQuit = true;
					break;
				}
			}
			
			if (shouldQuit)
				break;
			
			UpdateGame(lastFrameTime.count() * 1E-9f);
			
			if (fences[frameQueueIndex].IsNull())
			{
				fences[frameQueueIndex] = CreateVkFence();
			}
			else
			{
				vkWaitForFences(vulkan.device, 1, &*fences[frameQueueIndex], true, UINT64_MAX);
			}
			
			//Checks if the drawable size has changed.
			int drawableWidth, drawableHeight;
			SDL_Vulkan_GetDrawableSize(window, &drawableWidth, &drawableHeight);
			if (drawableWidth != currentDrawableWidth || drawableHeight != currentDrawableHeight)
			{
				vkDeviceWaitIdle(vulkan.device);
				
				SwapChain::Create(true);
				
				framebuffer.Create(renderer, drawableWidth, drawableHeight);
				
				SwapChain::PresentImage presentImage;
				framebuffer.GetPresentImage(presentImage);
				SwapChain::SetPresentImage(presentImage);
				
				currentDrawableWidth = drawableWidth;
				currentDrawableHeight = drawableHeight;
			}
			
			ProcessVulkanDestroyList();
			
			VkSemaphore signalSemaphore = *signalSemaphores[frameQueueIndex];
			renderer.Render({ &framebuffer, signalSemaphore, *fences[frameQueueIndex] });
			
			SwapChain::Present(signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			
			IncrementFrameIndex();
			
			lastFrameTime = std::chrono::high_resolution_clock::now() - frameBeginTime;
		}
		
		vkDeviceWaitIdle(vulkan.device);
		
		ClearVulkanDestroyList();
	}
}
