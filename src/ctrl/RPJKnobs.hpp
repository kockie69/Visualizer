#include "rack.hpp"

struct RPJKnobBig : RoundKnob {
	RPJKnobBig() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/RPJKnobBig.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/RPJKnobBig_bg.svg")));
	}
};

struct RPJKnobSmall : RoundKnob {
	RPJKnobSmall() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/RPJKnobSmall.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/RPJKnobSmall_bg.svg")));
	}
};

struct RPJKnob : RoundKnob {
	RPJKnob() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/RPJKnob.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/RPJKnob_bg.svg")));
	}
};


struct RPJKnobSnap : RPJKnob{
	RPJKnobSnap() {
		snap = true;
	}
};

struct Toggle2P : SvgSwitch {
	Toggle2P() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SW_Toggle_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SW_Toggle_2.svg")));

		// no shadow for switches
		shadow->opacity = 0.0f;
	}

	void onChange(const event::Change &e) override {

		SvgSwitch::onChange(e);

		if (getParamQuantity()->getValue() > 0.5f)
			getParamQuantity()->setValue(1.0f);
		else
			getParamQuantity()->setValue(0.0f);
	}
};