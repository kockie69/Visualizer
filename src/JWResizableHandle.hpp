#pragma once
#include "rack.hpp"

using namespace rack;

struct JWModuleResizeHandle : OpaqueWidget {
	Vec dragPos;
	Rect originalBox;
	GLFWwindow *window;

	JWModuleResizeHandle(GLFWwindow *w) {
		box.size = Vec(RACK_GRID_WIDTH * 1, RACK_GRID_HEIGHT);
		window = w;
	}

	void onDragStart(const event::DragStart &e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		dragPos = APP->scene->mousePos;
		ModuleWidget *mw = getAncestorOfType<ModuleWidget>();
		assert(mw);
		originalBox = mw->box;
	}

	void onDragMove(const event::DragMove &e) override {
		ModuleWidget *mw = getAncestorOfType<ModuleWidget>();
		assert(mw);

		Vec newDragPos = APP->scene->mousePos;
		float deltaX = newDragPos.x - dragPos.x;

		Rect newBox = originalBox;
		Rect oldBox = mw->box;
		const float minWidth = RENDER_WIDTH + 3*RACK_GRID_WIDTH;

		newBox.size.x += deltaX;
		newBox.size.x = std::fmax(newBox.size.x, minWidth);
		newBox.size.x = std::round(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;

		// Set box and test whether it's valid
		// When too large nanovg function will crash, so set limit of max size
			mw->box = newBox;
			if (!APP->scene->rack->requestModulePos(mw, newBox.pos)) {
				mw->box = oldBox;
			}
			glfwSetWindowSize(window,mw->box.size.x-85,mw->box.size.y);
	}
};