#include "RPJ.hpp"
#include "Visualizer.hpp"
#include "ctrl/RPJKnobs.hpp"

Display::Display() {
	prevZoomlevel= 0;
}

Display::~Display() {
	projectm_destroy(module->projectMHandle);
	module->projectMHandle = NULL;
}

void Display::step() {

	float zoomLevel= APP->scene->rackScroll->getZoom();
	if (prevZoomlevel != zoomLevel) {
		prevZoomlevel = zoomLevel;
		
		renderWidth = WIDTH * zoomLevel;
		renderHeight = HEIGHT * zoomLevel;
		if (module) {
			if (!module->projectMHandle)
				return;
			else 			
				projectm_set_window_size(module->projectMHandle, renderWidth, renderHeight);
		}
	}
	if (module) {
		if (module->projectMHandle) {
			unsigned int currentIndex;
			if (projectm_get_selected_preset_index(module->projectMHandle,&currentIndex))
				if ((unsigned int)module->index != module->currentIndex) 
					projectm_select_preset(module->projectMHandle,(unsigned int)module->index,module->hard_cut);
		}
	}
	// Render every frame
	dirty = true;

	FramebufferWidget::step();
}

void Display::drawFramebuffer() {
	if (!module->projectMHandle)
	{
    	return;
	}
	else {
		projectm_pcm_add_float_2ch_data(module->projectMHandle,module->pcmData,2);
		projectm_render_frame(module->projectMHandle);
	}
}

RPJVisualizer::RPJVisualizer() {

	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	configParam(PARAM_DURATION, 1.f, 100.f,10.f, "Preset duration playing"," seconds");
	configSwitch(PARAM_LOCK, 0.f, 1.f, 1.f, "Preset lock mode", {"Locked", "Unlocked"});
	index=0;	
	locked = false;
	lightDivider.setDivision(16);
	hard_cut=true;

	settings = projectm_alloc_settings();

	// Window/rendering settings
	settings->window_width = 150;
	settings->window_height = 150;
	settings->fps =  60;
	settings->mesh_x = 220;
	settings->mesh_y = 125;
	settings->aspect_correction = true;

	// Preset display settings
	settings->preset_duration = 30;
	settings->soft_cut_duration = 3;
	settings->hard_cut_enabled = hard_cut;
	settings->hard_cut_duration = 20;
	settings->hard_cut_sensitivity =  1.0;
	settings->beat_sensitivity = 1.0;
	settings->shuffle_enabled = true;
	std::string thePath = asset::plugin(pluginInstance, "res\\presets_projectM");
	settings->preset_url = (char *)thePath.c_str();
	// Unsupported settings
	settings->soft_cut_ratings_enabled = false;
	settings->menu_font_url = nullptr;
	settings->title_font_url = nullptr;

	projectMHandle = projectm_create_settings(settings, PROJECTM_FLAG_NONE);

	// Returns a list of all presets currently loaded by projectM

  	unsigned int n;
	n = projectm_get_playlist_size(projectMHandle);

  	if (n) {
  		for (unsigned int i = 0; i < n; ++i){
    		std::string s;   		
      		s = projectm_get_preset_name(projectMHandle,i);  		
    		presets.push_back(std::string(s));
		}
	}
}

void RPJVisualizer::process(const ProcessArgs &args) {

	pcmData[0] = inputs[RPJVisualizer::INPUT_INL].getVoltage();
	pcmData[1] = inputs[RPJVisualizer::INPUT_INR].getVoltage();
	
	projectm_set_preset_duration(projectMHandle,params[PARAM_DURATION].getValue());

	bool locked = params[PARAM_LOCK].getValue() <= 0.f;
	projectm_lock_preset(projectMHandle,locked);
	
	if (lightDivider.process()) {
		lights[LOCK_LIGHT].setBrightness(locked);
	}
}

json_t *RPJVisualizer::dataToJson() {
	json_t *rootJ=json_object();
	json_object_set_new(rootJ, "Locked", json_boolean(this->locked));
	json_object_set_new(rootJ, "Preset", json_integer(this->index));
	return rootJ;
}

void RPJVisualizer::dataFromJson(json_t *rootJ) {
	json_t *nLockedJ = json_object_get(rootJ, "Locked");
	json_t *nPresetJ = json_object_get(rootJ, "Preset");
	if (nLockedJ) {
		this->locked = json_boolean_value(nLockedJ);
	}
	if (nPresetJ) {
		this->index = json_integer_value(nPresetJ);
	}
}

struct VisualizerModuleWidget : ModuleWidget {
	VisualizerModuleWidget(RPJVisualizer* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		box.size = Vec(MODULE_WIDTH*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		
		Display *display = new Display();
		display->box.pos = Vec(80, 5);
		display->box.size = Vec(WIDTH, HEIGHT);
		display->module = module;
		addChild(display);

		// Then do the knobs
		const float knobX1 = 11;
		const float knobX2 = 27;
		const float knobX3 = 47;

		const float knobY1 = 33;
		const float knobY2 = 311;
		//const float knobY3 = 122;
		//const float knobY4 = 150;
		//const float knobY5 = 178;
		//const float knobY6 = 206;

		const float buttonX1 = 41;

		const float buttonY1 = 275;

		addParam(createParam<RPJKnob>(Vec(knobX2, knobY1), module, RPJVisualizer::PARAM_DURATION));		
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY1), module, RPJVisualizer::PARAM_LOCK, RPJVisualizer::LOCK_LIGHT));
		addInput(createInput<PJ301MPort>(Vec(knobX1, knobY2), module, RPJVisualizer::INPUT_INL));	
		addInput(createInput<PJ301MPort>(Vec(knobX3, knobY2), module, RPJVisualizer::INPUT_INR));	
	}

	void appendContextMenu(Menu *menu) override {
		RPJVisualizer *module = dynamic_cast<RPJVisualizer*>(this->module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createIndexPtrSubmenuItem("PlayList", module->presets, &module->index));

	}
};

Model * modelVisualizer = createModel<RPJVisualizer, VisualizerModuleWidget>("RPJVisualizer");
