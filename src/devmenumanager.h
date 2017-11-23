#pragma once

#include <glm/glm.hpp>

#include "ui/uidrawlist.h"
#include "rendering/renderer.h"

namespace MCR
{
	void InitDevMenu(Renderer& renderer);
	void DestroyDevMenu();
	
	void RenderDevMenu(UIDrawList& drawList, glm::ivec2 screenSize);
	void DevMenuKeyPressEvent(const SDL_KeyboardEvent& keyEvent);
}
