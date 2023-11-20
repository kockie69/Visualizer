#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;
extern Model *modelWindowedLFMModule;
extern Model *modelEmbeddedLFMModule;

struct BGPanel : Widget {
	Widget *panelBorder;
	NVGcolor color;

	BGPanel(NVGcolor _color) {
		panelBorder = new PanelBorder;
		color = _color;
		addChild(panelBorder);
	}

	void step() override {
		panelBorder->box.size = box.size;
		Widget::step();
	}

	void draw(const DrawArgs &args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 85.0, 0.0, box.size.x, box.size.y);
		nvgFillColor(args.vg, color);
		nvgFill(args.vg);
		Widget::draw(args);
	}
};