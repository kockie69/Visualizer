#include "RPJ.hpp"
#include "Visualizer.hpp"
#include "ctrl/RPJKnobs.hpp"

void myRenderer::draw() {

	if (projectMHandle)
		projectm_render_frame(projectMHandle);
}

void myRenderer::handlePCMData(float pcmData[2]) {
	if (projectMHandle)
		projectm_pcm_add_float_2ch_data(projectMHandle,pcmData,2);
}

void myRenderer::handlePreset(int i,int c,bool h) {

	if (projectMHandle) {
	
		if (i!=c)
			projectm_select_preset(projectMHandle,i,h);
	}
}

void myRenderer::handleWindowSize(float rx,float ry) {
	if (!projectMHandle)
		return;
	Vec size = APP->window->getSize();
	float zoomLevel= APP->scene->rackScroll->getZoom();
	bool changing = ((prevZoomlevel != zoomLevel) || (prevRx != rx) || (prevRy != ry));
	if (changing) {
		changed=true;
		prevZoomlevel = zoomLevel;
		prevRx = rx;
		prevRy =ry;
	}
	if (changed && !changing) {
		renderWidth = rx*380*zoomLevel;
		renderHeight = ry*380*zoomLevel;

		//projectm_set_window_size(projectMHandle, renderWidth, renderHeight);
		projectm_set_window_size(projectMHandle, rx*380*zoomLevel, ry*380*zoomLevel);
		changed=false;
		changeProcessed=true;
	}
}

std::vector<std::string> myRenderer::getPresets() {
	unsigned int n;
	std::vector<std::string> presets;
	n = projectm_get_playlist_size(projectMHandle);

  	if (!n)
	  return presets;
	else {
  		for (unsigned int i = 0; i < n; ++i){
    		std::string s;   		
      		s = projectm_get_preset_name(projectMHandle,i);  		
    		presets.push_back(s);
		}
	}
	return presets;
}

void myRenderer::nextPreset(bool n,bool h) {
	if (n)
		projectm_select_next_preset(projectMHandle,h);
}

void myRenderer::prevPreset(bool p,bool h) {
	if (p)
		projectm_select_previous_preset(projectMHandle,h);
}

void myRenderer::handleLocked(bool l) {
	projectm_lock_preset(projectMHandle,l);
}

void myRenderer::handleHardCut(bool h) {
	projectm_set_hard_cut_enabled(projectMHandle,h);
}

std::string myRenderer::getNamePreset(projectm_handle pm) {
	return getPresets()[index];
}

std::string myRenderer::getName() {
	return getNamePreset(projectMHandle);
}

void myRenderer::init(projectm_handle h) {
//	window = glfwCreateWindow(640, 480, "My Title",  NULL, NULL);
//	renderThread = std::thread([this,h](){ this->process(h); });
}

projectm_handle myRenderer::initSettings() {
	projectm_settings *settings = projectm_alloc_settings();

	// Window/rendering settings
	settings->window_width = 700;
	settings->window_height = 700;
	settings->fps =  60;
	settings->mesh_x = 220;
	settings->mesh_y = 125;
	settings->aspect_correction = true;

	// Preset display settings
	settings->preset_duration = 30;
	settings->soft_cut_duration = 3;
	settings->hard_cut_enabled = true;
	settings->hard_cut_duration = 20;
	settings->hard_cut_sensitivity =  1.0;
	settings->beat_sensitivity = 1.0;
	settings->shuffle_enabled = true;
	std::string thePath = asset::plugin(pluginInstance, "res\\presets_projectM\\");
	settings->preset_url = (char *)thePath.c_str();
	// Unsupported settings
	settings->soft_cut_ratings_enabled = false;
	settings->menu_font_url = nullptr;
	settings->title_font_url = nullptr;
	projectMHandle = projectm_create_settings(settings, PROJECTM_FLAG_NONE);
		
	return projectMHandle;
}

/*void myRenderer::process(projectm_handle h) {
	projectMHandle=h;
	//Initialize(s_ptr);
	prevZoomlevel= 0;
	index=0;
	if (!window)
		return;
	glfwMakeContextCurrent(window);

	//getPresets();
	while (true) {
		handleWindowSize();
		draw();
	}
	
}*/

myRenderer::myRenderer() {
	index=0;
}

/*void myRenderer::Initialize(projectm_settings *s) {
	prevZoomlevel= 0;
	index=0;
	projectMHandle = projectm_create_settings(s, PROJECTM_FLAG_DISABLE_PLAYLIST_LOAD);	
}*/

myRenderer::~myRenderer() {
	//renderThread.join();
	//projectm_free_settings(settings);
	projectm_destroy(projectMHandle);
	projectMHandle = NULL;
}

Display::Display() {
//	if (module)
	renderer.init(renderer.initSettings());
	oldIndex=-1;
}

Display::~Display() {

}

void Display::step() {
	if (module) {
		projectm_set_shuffle_enabled(renderer.projectMHandle,module->shuffleEnabled);
		projectm_set_preset_duration(renderer.projectMHandle,module->timer);
		renderer.handleWindowSize(module->resizeX,module->resizeY);
		unsigned int *currentIndex = (unsigned int *)malloc(sizeof(currentIndex));
		if (projectm_get_selected_preset_index(renderer.projectMHandle,currentIndex)) {
			if ((unsigned int)module->index != *(currentIndex)) {
				if (oldIndex==module->index)
					oldIndex=*(currentIndex);
				else
					oldIndex=module->index;
				projectm_select_preset(renderer.projectMHandle,oldIndex,module->hard_cut);
				module->index = oldIndex;
			}
		}
		free(currentIndex);
		
		projectm_pcm_add_float_2ch_data(renderer.projectMHandle,module->pcmData,2);
		
		renderer.nextPreset(module->next,module->hard_cut);
		module->next=false;
		renderer.prevPreset(module->prev,module->hard_cut);
		module->prev=false;
		projectm_lock_preset(renderer.projectMHandle,module->locked);
		projectm_set_hard_cut_enabled(renderer.projectMHandle,module->hard_cut);
		module->presetName = renderer.getName();
		//module->presetSize = renderer.getPresets().size();
	}
	// Render every frame
	dirty = true;

	OpenGlWidget::step();
}

void Display::drawFramebuffer() {
	//renderer.draw();

	if (renderer.projectMHandle) {
		math::Vec fbSize = getFramebufferSize();
		float x = widget->getSize().x;
		float y = widget->getSize().y;
		this->setSize(Vec(module->resizeX*380,module->resizeX*380));
		this->setPosition(Vec(85+module->posX, module->posY));
		//this->setPosition(Vec(80,0));
		//this->setSize(Vec(-85+widget->getSize().x,widget->getSize().y));
		projectm_render_frame(renderer.projectMHandle);
	}
}

RPJVisualizer::~RPJVisualizer() {

}

RPJVisualizer::RPJVisualizer() {

	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	configSwitch(PARAM_NEXT, 0.f, 1.f, 1.f, "Next preset");
	configSwitch(PARAM_PREV, 0.f, 1.f, 1.f, "Previous preset");
	configSwitch(PARAM_LOCK, 0.f, 1.f, 1.f, "Preset lock mode", {"Locked", "Unlocked"});
	configSwitch(PARAM_HARD_CUT, 0.f, 1.f, 1.f, "Hard cut mode", {"Enabled", "Disabled"});
	configParam(PARAM_RESIZE_X, 0.f, 1.f, 1.f, "Resize Preset X-axis");
	configParam(PARAM_RESIZE_Y, 0.f, 1.f, 1.f, "Resize Preset Y-axis");
	configParam(PARAM_POS_X, 0.f, 379.f, 0.f, "Position Preset X-axis");
	configParam(PARAM_POS_Y, 0.f, 379.f, 0.f, "Position Preset Y-axis");
	configParam(PARAM_TIMER, 0.f, 300.f, 30.f, "Time till next preset","Seconds");
	configButton(PARAM_NEXT, "Next preset");
	configButton(PARAM_PREV, "Previous preset");
	locked = false;
	lightDivider.setDivision(16);

	// Returns a list of all presets currently loaded by projectM
	next=false;
	prev=false;
	hard_cut=false;
	index=-1;
}

void RPJVisualizer::process(const ProcessArgs &args) {

	pcmData[0] = inputs[RPJVisualizer::INPUT_INL].getVoltage();
	pcmData[1] = inputs[RPJVisualizer::INPUT_INR].getVoltage();
	
	locked = params[PARAM_LOCK].getValue() <= 0.f;
	
	hard_cut = params[PARAM_HARD_CUT].getValue() <= 0.f;
	
	timer = params[PARAM_TIMER].getValue();

	resizeX = params[PARAM_RESIZE_X].getValue();
	resizeY = params[PARAM_RESIZE_Y].getValue();

	posX = params[PARAM_POS_X].getValue();
	posY = params[PARAM_POS_Y].getValue();
	//resizeX = 1;
	//resizeY = 1;

	if (nextBoolTrigger.process(params[PARAM_NEXT].getValue() > 0.f) || nextSchmittTrigger.process(inputs[INPUT_NEXT].getVoltage()) > 0.f) {
		next=true;
		nextLight=true;
	}

	if (params[PARAM_NEXT].getValue() == 0.f )
		nextLight=false;

	if (prevBoolTrigger.process(params[PARAM_PREV].getValue() > 0.f) || prevSchmittTrigger.process(inputs[INPUT_PREV].getVoltage()) > 0.f) {
		prevLight=true;
		prev=true;
	}

	if (params[PARAM_PREV].getValue() == 0.f)
		prevLight=false;

	if (lightDivider.process()) {
		lights[LOCK_LIGHT].setBrightness(locked);
		lights[HARD_CUT_LIGHT].setBrightness(hard_cut);
	}

	lights[NEXT_LIGHT].setBrightness(nextLight);
	lights[PREV_LIGHT].setBrightness(prevLight);
}

json_t *RPJVisualizer::dataToJson() {
	json_t *rootJ=json_object();
	json_object_set_new(rootJ, "Preset", json_integer(this->index));
	json_object_set_new(rootJ, "Shuffle", json_boolean(this->shuffleEnabled));
	return rootJ;
}

void RPJVisualizer::dataFromJson(json_t *rootJ) {
	json_t *nPresetJ = json_object_get(rootJ, "Preset");
	json_t *nShuffleJ = json_object_get(rootJ, "Shuffle");
	if (nPresetJ) {
		this->index = json_integer_value(nPresetJ);
	}
	if (nShuffleJ) {
		this->shuffleEnabled = json_boolean_value(nShuffleJ);
	}
}

struct VisualizerModuleWidget : ModuleWidget {
	Display *display;

	VisualizerModuleWidget(RPJVisualizer* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, 365)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		box.size = Vec(MODULE_WIDTH*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		
		display = new Display();
		display->box.pos = Vec(80, 0);
		display->box.size = Vec(WIDTH, HEIGHT);
		//display->setSize(Vec(WIDTH, HEIGHT));
		display->module = module;
		display->widget = this;
		addChild(display);

		
		//PresetNameDisplay * pnd = new PresetNameDisplay(Vec(29,130));
		//pnd->module = module;
		//addChild(pnd);
		
		// Then do the knobs
		const float knobX1 = 10;
		const float knobX2 = 27;
		const float knobX3 = 45;
		const float knobY1 = 45;
		const float knobY2 = 95;
		const float knobY3 = 137;
		//const float knobY4 = 150;
		//const float knobY5 = 178;
		//const float knobY6 = 206;

		addParam(createParam<RPJKnob>(Vec(knobX1,knobY1), module, RPJVisualizer::PARAM_POS_X));
		addParam(createParam<RPJKnob>(Vec(knobX3,knobY1), module, RPJVisualizer::PARAM_POS_Y));
		addParam(createParam<RPJKnob>(Vec(knobX1,knobY2), module, RPJVisualizer::PARAM_RESIZE_X));
		addParam(createParam<RPJKnob>(Vec(knobX3,knobY2), module, RPJVisualizer::PARAM_RESIZE_Y));
		addParam(createParam<RPJKnob>(Vec(knobX2,knobY3), module, RPJVisualizer::PARAM_TIMER));

		const float buttonX1 = 41;
		const float buttonX2 = 61;

		const float buttonY1 = 185;
		const float buttonY2 = 215;
		const float buttonY3 = 245;
		const float buttonY4 = 275;

		const float jackX1 = 11;
		const float jackX2 = 47;

		const float jackY1 = 173;
		const float jackY2 = 203;
		const float jackY3 = 311;

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX2,buttonY1), module, RPJVisualizer::PARAM_NEXT,RPJVisualizer::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX2,buttonY2), module, RPJVisualizer::PARAM_PREV,RPJVisualizer::PREV_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY3), module, RPJVisualizer::PARAM_HARD_CUT, RPJVisualizer::HARD_CUT_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY4), module, RPJVisualizer::PARAM_LOCK, RPJVisualizer::LOCK_LIGHT));
		
		addInput(createInput<PJ301MPort>(Vec(jackX1, jackY1), module, RPJVisualizer::INPUT_NEXT));	
		addInput(createInput<PJ301MPort>(Vec(jackX1, jackY2), module, RPJVisualizer::INPUT_PREV));	

		addInput(createInput<PJ301MPort>(Vec(jackX1, jackY3), module, RPJVisualizer::INPUT_INL));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY3), module, RPJVisualizer::INPUT_INR));	
	}

	void appendContextMenu(Menu *menu) override {
		RPJVisualizer *module = dynamic_cast<RPJVisualizer*>(this->module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createIndexPtrSubmenuItem("PlayList", display->renderer.getPresets(), &module->index));
		menu->addChild(createBoolPtrMenuItem("Shuffle enabled","",&module->shuffleEnabled));
	}
};

Model * modelVisualizer = createModel<RPJVisualizer, VisualizerModuleWidget>("RPJVisualizer");
