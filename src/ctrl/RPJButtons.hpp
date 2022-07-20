#include "rack.hpp"

struct ButtonBigSwitch : SvgSwitch  {
	ButtonBigSwitch() {
		momentary=false;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonLarge_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonLarge_1.svg")));
	}
	
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct ButtonBig : SvgSwitch  {
	ButtonBig() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonLarge_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonLarge_1.svg")));
	}
	
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct ButtonMinBig : SvgSwitch  {
	ButtonMinBig() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonMinLarge_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonMinLarge_1.svg")));
	}
	
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct ButtonPlusBig : SvgSwitch  {
	ButtonPlusBig() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonPlusLarge_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonPlusLarge_1.svg")));
	}
	
	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct buttonPlusSmall : SvgSwitch  {
	buttonPlusSmall() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonPlus_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonPlus_1.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}

};

struct buttonMinSmall : SvgSwitch  {
	buttonMinSmall() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonMin_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/ButtonMin_1.svg")));
	}
	
	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}

};

class ToggleButton : public ::rack::app::SvgSwitch
{
public:
    ::rack::app::CircularShadow* shadowToDelete = nullptr;

    ~ToggleButton()
    {
        if (shadowToDelete) {
            delete shadowToDelete;
        }
    }
    
    ToggleButton()
    {
        // The default shadow gives a look we don't want 
        auto shadowToDelete = this->shadow;
        this->fb->removeChild(shadowToDelete);

        // old one had default size 0
        this->box.size.y = 0;
        this->box.size.x = 0;
    }

    // Old switch took relative paths into plugin bundle
    void addSvg(const char* resPath)
    {
        addFrame(APP->window->loadSvg(resPath));
    }

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

class RedButton : public ToggleButton {
public:
    RedButton() {
		rack::app::Switch::momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/oval-button-up-grey.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/oval-button-down.svg")));
    }
};

struct StartButton : ToggleButton  {
	
	StartButton() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Start_line_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Start_line_on.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct StopButton : ToggleButton  {

	StopButton() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Stop_line_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Stop_line_on.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct PauseButton : ToggleButton  {

	PauseButton() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Pause_line_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Pause_line_on.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};


struct FwdButton : ToggleButton  {

	FwdButton() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Fwd_line_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Fwd_line_on.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct RwdButton : ToggleButton  {

	RwdButton() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Rwd_line_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Rwd_line_on.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};

struct EjectButton : ToggleButton  {

	EjectButton() {
		momentary=true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Ejct_line_off.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/buttons/Ejct_line_on.svg")));
	}

	void drawLayer(const DrawArgs &args, int layer)override {
		if (layer == 1) {
			SvgSwitch::draw(args);
		}
		SvgSwitch::drawLayer(args,layer);
	}
};
