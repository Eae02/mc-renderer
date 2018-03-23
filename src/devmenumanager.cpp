#include "devmenumanager.h"
#include "ui/devmenubar.h"
#include "ui/profilingpane.h"
#include "timemanager.h"

#include <memory>

namespace MCR
{
	static std::unique_ptr<DevMenuBar> devMenuBar;
	
	static float g_timeScale = 100;
	
	void InitDevMenu(Renderer& renderer, ProfilingPane& profilingPane, TimeManager& timeManager)
	{
		devMenuBar = std::make_unique<DevMenuBar>();
		
		DevMenu rendererMenu;
		rendererMenu.AddValue<bool>("Wireframe", [&] { return renderer.Wireframe(); },
		                            [&] (bool wireframe) { renderer.SetWireframe(wireframe); });
		
		rendererMenu.AddValue<bool>("Freeze Frustum", [&] { return renderer.IsFrustumFrozen(); },
		                            [&] (bool frustumFrozen) { renderer.SetFrustumFrozen(frustumFrozen); });
		
		auto waterMenu = std::make_unique<DevMenu>();
		waterMenu->AddValue<bool>("Wireframe", [&] { return renderer.WireframeWater(); },
		                          [&] (bool wireframe) { renderer.SetWireframeWater(wireframe); });
		rendererMenu.AddSubMenu("Water", std::move(waterMenu));
		
		rendererMenu.AddAction("Capture Shadow Volume", [&] { return renderer.CaptureShadowVolume(); });
		rendererMenu.AddAction("Clear Shadow Volume", [&] { return renderer.ClearShadowVolume(); });
		
		rendererMenu.AddAction("Capture Vis. Graph", [&] { return renderer.CaptureVisibilityGraph(); });
		rendererMenu.AddAction("Clear Vis. Graph", [&] { return renderer.ClearVisibilityGraph(); });
		
		devMenuBar->AddMenu("Renderer", std::make_unique<DevMenu>(std::move(rendererMenu)));
		
		DevMenu profilingMenu;
		profilingMenu.AddValue<bool>("Show Profiling Pane", [&] { return profilingPane.IsVisible(); },
		                             [&] (bool wireframe) { profilingPane.SetVisible(wireframe); });
		profilingMenu.AddAction("Save Profiling Data", [&] { profilingPane.DumpToCSV(); });
		
		devMenuBar->AddMenu("Profiling", std::make_unique<DevMenu>(std::move(profilingMenu)));
		
		DevMenu timeMenu;
		timeMenu.AddValue<float>("Timescale", [&] { return g_timeScale; },
		                         [&] (float timeScale) { timeManager.SetTimeScale(g_timeScale = timeScale); });
		timeMenu.AddValue<bool>("Freeze Time", [&] { return timeManager.IsTimeFrozen(); },
		                         [&] (bool freezeTime) { timeManager.SetFreezeTime(freezeTime); });
		
		devMenuBar->AddMenu("Time", std::make_unique<DevMenu>(std::move(timeMenu)));
	}
	
	void DestroyDevMenu()
	{
		devMenuBar.reset();
	}
	
	void RenderDevMenu(UIDrawList& drawList, glm::ivec2 screenSize)
	{
		devMenuBar->Render(drawList, screenSize);
	}
	
	void DevMenuKeyPressEvent(const SDL_KeyboardEvent& keyEvent)
	{
		devMenuBar->KeyPressEvent(keyEvent);
	}
}
