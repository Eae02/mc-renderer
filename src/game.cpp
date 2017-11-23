#include "game.h"
#include "blocks/blockstexturemanager.h"
#include "blocks/registerblocktypes.h"
#include "vulkan/vkutils.h"
#include "inputstate.h"
#include "timemanager.h"
#include "devmenumanager.h"
#include "world/worldmanager.h"
#include "ui/font.h"
#include "ui/uigraphicscontext.h"
#include "ui/uidrawlist.h"
#include "rendering/postprocessor.h"
#include "rendering/renderer.h"
#include "rendering/framebuffer.h"
#include "rendering/causticstexture.h"
#include "rendering/windnoiseimage.h"
#include "rendering/regions/watermesh.h"
#include "profiling/profiling.h"
#include "ui/profilingpane.h"

#include <cstdint>
#include <chrono>

namespace MCR
{
	std::unique_ptr<WorldManager> worldManager;
	
	TimeManager timeManager;
	
#ifdef MCR_DEBUG
	ProfilingPane profilingPane;
#endif
	
	void UpdateGame(float dt, const InputState& inputState)
	{
		{
			MCR_SCOPED_TIMER(0, "World Update");
			worldManager->Update(dt, inputState);
		}
		
		timeManager.Update(dt, inputState);
	}
	
	struct FrameQueueEntry
	{
		bool m_hasSubmittedOnce;
		VkHandle<VkFence> m_fence;
		VkHandle<VkSemaphore> m_signalSemaphore;
		
#ifdef MCR_DEBUG
		FrameProfiler m_profiler;
#endif
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
		
		std::vector<FrameQueueEntry> frames(SwapChain::GetImageCount());
		
		for (FrameQueueEntry& frame : frames)
		{
			frame.m_hasSubmittedOnce = false;
			frame.m_fence = CreateVkFence();
			frame.m_signalSemaphore = CreateVkSemaphore();
		}
		
		auto startTime = std::chrono::high_resolution_clock::now();
		std::chrono::nanoseconds lastFrameTime = std::chrono::nanoseconds::zero();
		
		Settings settings;
		
		fs::path settingsPath = GetAppDataPath() / "settings.json";
		if (fs::exists(settingsPath))
		{
			settings.Load(settingsPath);
		}
		
		int cursorX, cursorY;
		SDL_GetMouseState(&cursorX, &cursorY);
		
		InputState inputState(glm::vec2(cursorX, cursorY));
		
		CausticsTexture::CreatePipelines();
		
		int currentDrawableWidth = 0;
		int currentDrawableHeight = 0;
		
		std::unique_ptr<LoadContext> loadContext = std::make_unique<LoadContext>();
		loadContext->Begin();
		
		//Loads block textures
		const fs::path texturePackPath = GetResourcePath() / "textures" / "chroma_hills.zip";
		BlocksTextureManager::SetInstance(std::make_unique<BlocksTextureManager>(
			BlocksTextureManager::LoadTexturePack(texturePackPath, *loadContext)));
		
		WindNoiseImage::SetInstance(std::make_unique<WindNoiseImage>(WindNoiseImage::Generate(256, *loadContext)));
		
		Font::LoadStandard(*loadContext);
		
		Renderer renderer;
		renderer.Initialize(*loadContext);
		renderer.SetShadowQualityLevel(settings.GetShadowQuality());
		
		loadContext->End();
		loadContext.reset();
		
		RegisterBlockTypes();
		
		WaterMesh::CreateBuffers();
		
		PostProcessor postProcessor;
		UIGraphicsContext uiGraphicsContext;
		Framebuffer framebuffer;
		
		UIDrawList uiDrawList;
		
		worldManager = std::make_unique<WorldManager>();
		worldManager->SetRenderDistance(settings.GetRenderDistance());
		renderer.SetWorldManager(worldManager.get());
		
		InitDevMenu(renderer);
		
		const fs::path worldPath = MCR::GetResourcePath() / "world";
		if (!fs::exists(worldPath))
		{
			fs::create_directory(worldPath);
		}
		
		worldManager->SetWorld(std::make_unique<World>(worldPath));
		
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
				
				case SDL_KEYDOWN:
					DevMenuKeyPressEvent(event.key);
					
					[[fallthrough]];
				case SDL_KEYUP:
					if (event.key.repeat)
						break;
					
					inputState.KeyEvent(event.key.keysym.scancode, event.key.state == SDL_PRESSED);
					break;
				}
			}
			
			if (shouldQuit)
				break;
			
			//Checks if the drawable size has changed.
			int drawableWidth, drawableHeight;
			SDL_GetWindowSize(window, &drawableWidth, &drawableHeight);
			if (drawableWidth != currentDrawableWidth || drawableHeight != currentDrawableHeight)
			{
				vkDeviceWaitIdle(vulkan.device);
				
				SwapChain::Create(settings.EnableVSync());
				
				Framebuffer::RenderPasses renderPasses;
				renderer.GetRenderPasses(renderPasses);
				renderPasses.m_ui = uiGraphicsContext.GetRenderPass();
				
				framebuffer.Create(renderPasses, SwapChain::GetImages(), drawableWidth, drawableHeight);
				
				renderer.FramebufferChanged(framebuffer);
				postProcessor.FramebufferChanged(framebuffer);
				
				currentDrawableWidth = drawableWidth;
				currentDrawableHeight = drawableHeight;
			}
			
			VkSemaphore acquireSemaphore;
			frameQueueIndex = SwapChain::AcquireImage(acquireSemaphore);
			
			uiDrawList.Reset();
			
			FrameQueueEntry& frame = frames[frameQueueIndex];
			
#ifdef MCR_DEBUG
			currentFrameProfiler = &frame.m_profiler;
			ProfilingData profilingData;
#endif
			
			UpdateGame(lastFrameTime.count() * 1E-9f, inputState);
			
			if (frame.m_hasSubmittedOnce)
			{
				{
					MCR_SCOPED_TIMER(0, "GPU sync");
					WaitForFence(*frame.m_fence);
					vkResetFences(vulkan.device, 1, &*frame.m_fence);
				}
				
#ifdef MCR_DEBUG
				profilingData = frame.m_profiler.GetData(lastFrameTime);
				currentFrameProfiler->NewFrame();
#endif
			}
			else
			{
				frame.m_hasSubmittedOnce = true;
			}
			
			ProcessVulkanDestroyList();
			WaterMesh::ProcessFreeList();
			
			{
				MCR_SCOPED_TIMER(0, "Render");
				renderer.Render({ timeF, &timeManager });
			}
			
#ifdef MCR_DEBUG
			profilingPane.FrameEnd(profilingData, inputState, uiDrawList);
			
			RenderDevMenu(uiDrawList, { drawableWidth, drawableHeight });
#endif
			
			{
				MCR_SCOPED_TIMER(0, "UI Render");
				uiGraphicsContext.Draw(uiDrawList, framebuffer);
			}
			
			const VkCommandBuffer commandBuffers[] =
			{
				renderer.GetCommandBuffer(),
				postProcessor.GetCommandBuffer(),
				uiGraphicsContext.GetCommandBuffer()
			};
			
			const VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			
			const VkSubmitInfo submitInfo = 
			{
				/* sType                */ VK_STRUCTURE_TYPE_SUBMIT_INFO,
				/* pNext                */ nullptr,
				/* waitSemaphoreCount   */ 1,
				/* pWaitSemaphores      */ &acquireSemaphore,
				/* pWaitDstStageMask    */ &waitStages,
				/* commandBufferCount   */ ArrayLength(commandBuffers),
				/* pCommandBuffers      */ commandBuffers,
				/* signalSemaphoreCount */ 1,
				/* pSignalSemaphores    */ &*frame.m_signalSemaphore
			};
			
			{
				MCR_SCOPED_TIMER(0, "Submit");
				vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Submit(1, &submitInfo, *frame.m_fence);
			}
			
			{
				MCR_SCOPED_TIMER(0, "Present");
				SwapChain::Present(*frame.m_signalSemaphore, frameQueueIndex);
			}
			
			IncrementFrameIndex();
			
			lastFrameTime = std::chrono::high_resolution_clock::now() - frameBeginTime;
		}
		
		settings.Save(settingsPath);
		
		renderer.SaveCausticsTexture();
		
		worldManager->SaveAll();
		worldManager->WaitIdle();
		
		vkDeviceWaitIdle(vulkan.device);
		
		Font::DestroyStandard();
		
		worldManager = nullptr;
		BlocksTextureManager::SetInstance(nullptr);
		WindNoiseImage::SetInstance(nullptr);
		
		ChunkBufferAllocator::s_instance.ReleaseMemory();
		
		CausticsTexture::DestroyPipelines();
		WaterMesh::DestroyBuffers();
		
		DestroyDevMenu();
		
		ClearVulkanDestroyList();
	}
}
