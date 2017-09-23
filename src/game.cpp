﻿#include "game.h"
#include "blocks/blockstexturemanager.h"
#include "blocks/registerblocktypes.h"
#include "vulkan/vkutils.h"
#include "inputstate.h"
#include "timemanager.h"
#include "world/worldmanager.h"
#include "ui/font.h"
#include "ui/uigraphicscontext.h"
#include "ui/uidrawlist.h"
#include "rendering/postprocessor.h"
#include "rendering/renderer.h"
#include "rendering/framebuffer.h"
#include "profiling/profiling.h"
#include "ui/profilingpane.h"

#include <SDL2/SDL_vulkan.h>
#include <cstdint>
#include <chrono>

namespace MCR
{
	std::unique_ptr<WorldManager> worldManager;
	
	TimeManager timeManager;
	
	ProfilingPane profilingPane;
	
	void Initialize()
	{
		LoadContext loadContext;
		loadContext.Begin();
		
		//Loads block textures
		const fs::path blocksTxtPath = GetResourcePath() / "textures" / "blocks" / "blocks.txt";
		BlocksTextureManager::SetInstance(std::make_unique<BlocksTextureManager>(BlocksTextureManager::LoadTextures(blocksTxtPath, loadContext)));
		
		Font::LoadStandard(loadContext);
		
		loadContext.End();
		
		RegisterBlockTypes();
	}
	
	void UpdateGame(float dt, const InputState& inputState)
	{
		{
			MCR_SCOPED_TIMER(0, "World Update")
			worldManager->Update(dt, inputState);
		}
		
		timeManager.Update(dt);
	}
	
	struct FrameQueueEntry
	{
		VkHandle<VkFence> m_fence;
		VkHandle<VkSemaphore> m_signalSemaphore;
		FrameProfiler m_profiler;
	};
	
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
		
		std::array<FrameQueueEntry, MaxQueuedFrames> frames;
		
		for (FrameQueueEntry& frame : frames)
		{
			frame.m_fence = CreateVkFence();
			frame.m_signalSemaphore = CreateVkSemaphore();
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
		PostProcessor postProcessor;
		UIGraphicsContext uiGraphicsContext;
		Framebuffer framebuffer;
		
		UIDrawList uiDrawList;
		
		postProcessor.SetRenderSettings(renderer.GetRenderSettingsBufferInfo());
		
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
			
			uiDrawList.Reset();
			
			FrameQueueEntry& frame = frames[frameQueueIndex];
			currentFrameProfiler = &frame.m_profiler;
			
			UpdateGame(lastFrameTime.count() * 1E-9f, inputState);
			
			ProfilingData profilingData;
			
			if (frameIndex >= MaxQueuedFrames)
			{
				{
					MCR_SCOPED_TIMER(0, "GPU sync")
					CheckResult(vkWaitForFences(vulkan.device, 1, &*frame.m_fence, true, UINT64_MAX));
				}
				
				profilingData = frame.m_profiler.GetData();
			}
			
			//Checks if the drawable size has changed.
			int drawableWidth, drawableHeight;
			SDL_Vulkan_GetDrawableSize(window, &drawableWidth, &drawableHeight);
			if (drawableWidth != currentDrawableWidth || drawableHeight != currentDrawableHeight)
			{
				vkDeviceWaitIdle(vulkan.device);
				
				SwapChain::Create(true);
				
				framebuffer.Create(renderer, uiGraphicsContext, drawableWidth, drawableHeight);
				
				renderer.FramebufferChanged(framebuffer);
				postProcessor.FramebufferChanged(framebuffer);
				
				SwapChain::PresentImage presentImage;
				framebuffer.GetPresentImage(presentImage);
				SwapChain::SetPresentImage(presentImage);
				
				currentDrawableWidth = drawableWidth;
				currentDrawableHeight = drawableHeight;
			}
			
			ProcessVulkanDestroyList();
			
			renderer.Render({ timeF, &frame.m_profiler, &timeManager });
			
			profilingPane.FrameEnd(profilingData, inputState, uiDrawList);
			
			uiGraphicsContext.Draw(uiDrawList, framebuffer);
			
			const VkCommandBuffer commandBuffers[] =
			{
				renderer.GetCommandBuffer(),
				postProcessor.GetCommandBuffer(),
				uiGraphicsContext.GetCommandBuffer()
			};
			
			const VkSubmitInfo submitInfo = 
			{
				/* sType                */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
				/* pNext                */ nullptr,
				/* waitSemaphoreCount   */ 0,
				/* pWaitSemaphores      */ nullptr,
				/* pWaitDstStageMask    */ nullptr,
				/* commandBufferCount   */ ArrayLength(commandBuffers),
				/* pCommandBuffers      */ commandBuffers,
				/* signalSemaphoreCount */ 1,
				/* pSignalSemaphores    */ &*frame.m_signalSemaphore
			};
			
			{
				MCR_SCOPED_TIMER(0, "Submit")
				vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Submit(1, &submitInfo, *frame.m_fence);
			}
			
			{
				MCR_SCOPED_TIMER(0, "Present");
				SwapChain::Present(*frame.m_signalSemaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			}
			
			IncrementFrameIndex();
			
			lastFrameTime = std::chrono::high_resolution_clock::now() - frameBeginTime;
		}
		
		worldManager->SaveAll(true);
		
		vkDeviceWaitIdle(vulkan.device);
		
		Font::DestroyStandard();
		
		ChunkBufferAllocator::s_instance.ReleaseMemory();
		
		worldManager = nullptr;
		BlocksTextureManager::SetInstance(nullptr);
		
		ClearVulkanDestroyList();
	}
}
