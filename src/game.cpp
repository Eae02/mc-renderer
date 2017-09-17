#include "game.h"
#include "blocks/blockstexturemanager.h"
#include "blocks/registerblocktypes.h"
#include "vulkan/vkutils.h"
#include "inputstate.h"
#include "world/worldmanager.h"
#include "rendering/postprocessor.h"
#include "rendering/renderer.h"
#include "rendering/rendererframebuffer.h"

#include <SDL2/SDL_vulkan.h>
#include <cstdint>
#include <chrono>

namespace MCR
{
	std::unique_ptr<WorldManager> worldManager;
	
	void Initialize()
	{
		LoadContext loadContext;
		loadContext.Begin();
		
		//Loads block textures
		const fs::path blocksTxtPath = GetResourcePath() / "textures" / "blocks" / "blocks.txt";
		BlocksTextureManager::SetInstance(std::make_unique<BlocksTextureManager>(BlocksTextureManager::LoadTextures(blocksTxtPath, loadContext)));
		
		loadContext.End();
		
		RegisterBlockTypes();
	}
	
	void UpdateGame(float dt, const InputState& inputState)
	{
		worldManager->Update(dt, inputState);
	}
	
	void RunGameLoop(SDL_Window* window)
	{
		bool relativeMouseMode = false;
		
#ifdef MCR_DEBUG
		//Don't hide the mouse by default if attached to a debugger.
		if (!IsAttachedToDebugger())
#endif
		{
			relativeMouseMode = true;
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		
		VkHandle<VkFence> fences[MaxQueuedFrames];
		VkHandle<VkSemaphore> signalSemaphores[MaxQueuedFrames];
		
		for (size_t i = 0; i < MaxQueuedFrames; i++)
		{
			signalSemaphores[i] = CreateVkSemaphore();
		}
		
		auto startTime = std::chrono::high_resolution_clock::now();
		std::chrono::nanoseconds lastFrameTime = std::chrono::nanoseconds::zero();
		
		int cursorX, cursorY;
		SDL_GetMouseState(&cursorX, &cursorY);
		
		InputState inputState(glm::vec2(cursorX, cursorY));
		
		int currentDrawableWidth = 0;
		int currentDrawableHeight = 0;
		
		Initialize();
		
		Renderer renderer;
		RendererFramebuffer framebuffer;
		
		worldManager = std::make_unique<WorldManager>();
		renderer.SetWorldManager(worldManager.get());
		
		worldManager->SetWorld(World::CreateNew(MCR::GetResourcePath() / "world"));
		
		while (true)
		{
			auto frameBeginTime = std::chrono::high_resolution_clock::now();
			float timeF = std::chrono::duration_cast<std::chrono::nanoseconds>(frameBeginTime - startTime).count() * 1E-9f;
			
			inputState.NewFrame();
			
			bool shouldQuit = false;
			
			//Event loop
			SDL_Event event;
			while (SDL_PollEvent(&event) && !shouldQuit)
			{
				switch (event.type)
				{
				case SDL_QUIT:
					shouldQuit = true;
					break;
				case SDL_MOUSEMOTION:
					inputState.MouseMotionEvent(event.motion);
					break;
				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					inputState.MouseButtonEvent(event.button.button, event.button.state == SDL_PRESSED);
					break;
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					if (event.key.repeat)
						break;
					
					if (event.key.state == SDL_PRESSED)
					{
						if (event.key.keysym.scancode == SDL_SCANCODE_F7)
						{
							renderer.SetWireframe(!renderer.Wireframe());
						}
						else if (event.key.keysym.scancode == SDL_SCANCODE_F8)
						{
							renderer.SetFrustumFrozen(!renderer.IsFrustumFrozen());
						}
						else if (event.key.keysym.scancode == SDL_SCANCODE_F10)
						{
							relativeMouseMode = !relativeMouseMode;
							SDL_SetRelativeMouseMode(relativeMouseMode ? SDL_TRUE : SDL_FALSE);
						}
					}
					
					inputState.KeyEvent(event.key.keysym.scancode, event.key.state == SDL_PRESSED);
					break;
				}
			}
			
			if (shouldQuit)
				break;
			
			UpdateGame(lastFrameTime.count() * 1E-9f, inputState);
			
			if (fences[frameQueueIndex].IsNull())
			{
				fences[frameQueueIndex] = CreateVkFence();
			}
			else
			{
				CheckResult(vkWaitForFences(vulkan.device, 1, &*fences[frameQueueIndex], true, UINT64_MAX));
			}
			
			//Checks if the drawable size has changed.
			int drawableWidth, drawableHeight;
			SDL_Vulkan_GetDrawableSize(window, &drawableWidth, &drawableHeight);
			if (drawableWidth != currentDrawableWidth || drawableHeight != currentDrawableHeight)
			{
				vkDeviceWaitIdle(vulkan.device);
				
				SwapChain::Create(true);
				
				framebuffer.Create(renderer, drawableWidth, drawableHeight);
				
				renderer.FramebufferChanged(framebuffer);
				
				SwapChain::PresentImage presentImage;
				framebuffer.GetPresentImage(presentImage);
				SwapChain::SetPresentImage(presentImage);
				
				currentDrawableWidth = drawableWidth;
				currentDrawableHeight = drawableHeight;
			}
			
			ProcessVulkanDestroyList();
			
			VkSemaphore signalSemaphore = *signalSemaphores[frameQueueIndex];
			renderer.Render({ timeF, signalSemaphore, *fences[frameQueueIndex] });
			
			SwapChain::Present(signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			
			IncrementFrameIndex();
			
			lastFrameTime = std::chrono::high_resolution_clock::now() - frameBeginTime;
		}
		
		worldManager->SaveAll(true);
		
		vkDeviceWaitIdle(vulkan.device);
		
		worldManager = nullptr;
		BlocksTextureManager::SetInstance(nullptr);
		
		ClearVulkanDestroyList();
	}
}
