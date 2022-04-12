#include "RPJ.hpp"

const int WIDTH=380;
const int HEIGHT=380;

struct TestDisplay : OpenGlWidget {
	TestDisplay() {};
	~TestDisplay() {};
	void drawFramebuffer() override { OpenGlWidget::drawFramebuffer();};
	void step() override {OpenGlWidget::step();};
};

struct RPJTest : Module {

	enum ParamIds {
		PARAM_NEXT,
		PARAM_PREV,
		PARAM_LOCK,
		PARAM_HARD_CUT,
		PARAM_RESIZE_X,
		PARAM_RESIZE_Y,
		PARAM_POS_X,
		PARAM_POS_Y,
		PARAM_TIMER,
		NUM_PARAMS,
	};

	enum InputIds {
		INPUT_INL,
		INPUT_INR,
		INPUT_NEXT,
		INPUT_PREV,
		NUM_INPUTS,
	};

	enum OutputIds {
		NUM_OUTPUTS,
	};

	enum LightsIds {
		NEXT_LIGHT,
		PREV_LIGHT,
		LOCK_LIGHT,
		HARD_CUT_LIGHT,
		NUM_LIGHTS,
	};
    
    RPJTest() {
	    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }
};
 
struct TestModuleWidget : ModuleWidget {
	
    TestModuleWidget(RPJTest* module) {

	    setModule(module);
	    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

	    addChild(createWidget<ScrewSilver>(Vec(0, 0)));
	    addChild(createWidget<ScrewSilver>(Vec(0, 365)));
	
	    TestDisplay *testdisplay = new TestDisplay();
	    testdisplay->box.pos = Vec(85, 0);
		testdisplay->box.size = Vec( this->box.size.x-85, this->box.size.y);
		addChild(testdisplay);
    }
};

Model * modelTest = createModel<RPJTest, TestModuleWidget>("Test");