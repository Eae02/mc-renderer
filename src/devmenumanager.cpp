#include "devmenumanager.h"
#include "ui/devmenubar.h"
#include "ui/profilingpane.h"

#include <memory>

namespace MCR
{
	static std::unique_ptr<DevMenuBar> devMenuBar;
	
	void InitDevMenu(Renderer& renderer, ProfilingPane& profilingPane)
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
