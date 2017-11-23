#include "devmenumanager.h"
#include "ui/devmenubar.h"

#include <memory>

namespace MCR
{
	static std::unique_ptr<DevMenuBar> devMenuBar;
	
	void InitDevMenu(Renderer& renderer)
	{
		devMenuBar = std::make_unique<DevMenuBar>();
		
		DevMenu rendererMenu;
		rendererMenu.AddValue<bool>("Wireframe", [&] { return renderer.Wireframe(); },
		                            [&] (bool wireframe) { renderer.SetWireframe(wireframe); });
		
		rendererMenu.AddValue<bool>("Freeze Frustum", [&] { return renderer.IsFrustumFrozen(); },
		                            [&] (bool frustumFrozen) { renderer.SetFrustumFrozen(frustumFrozen); });
		
		rendererMenu.AddAction("Capture Shadow Volume", [&] { return renderer.CaptureShadowVolume(); });
		rendererMenu.AddAction("Clear Shadow Volume", [&] { return renderer.ClearShadowVolume(); });
		
		rendererMenu.AddAction("Capture Vis. Graph", [&] { return renderer.CaptureVisibilityGraph(); });
		rendererMenu.AddAction("Clear Vis. Graph", [&] { return renderer.ClearVisibilityGraph(); });
		
		devMenuBar->AddMenu("Renderer", std::make_unique<DevMenu>(std::move(rendererMenu)));
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
