#include "RPJ.hpp"
#include "Visualizer.hpp"

void myRenderer::draw() {

	if (projectMHandle)
		projectm_render_frame(projectMHandle);
}

void myRenderer::handlePCMData(float pcmData[2]) {
	if (projectMHandle)
		projectm_pcm_add_float_2ch_data(projectMHandle,pcmData,2);
}

void myRenderer::handlePreset() {

	if (projectMHandle) {
		projectm_set_hard_cut_enabled(projectMHandle,hard_cut);
		projectm_lock_preset(projectMHandle,locked);	
		
		if (oldIndex!=index) {
			projectm_select_preset(projectMHandle,index,hard_cut);
			oldIndex=index;
		}
		//projectm_set_title(projectMHandle, projectm_get_title(projectMHandle));
	}
}

void myRenderer::handleWindowSize() {
	float zoomLevel= APP->scene->rackScroll->getZoom();
	if (prevZoomlevel != zoomLevel) {
		prevZoomlevel = zoomLevel;
		
		renderWidth = WIDTH * zoomLevel;
		renderHeight = HEIGHT * zoomLevel;
		if (!projectMHandle)
			return;
		else 			
			projectm_set_window_size(projectMHandle, renderWidth, renderHeight);
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

void myRenderer::nextPreset(bool next) {
	if (next)
		projectm_select_next_preset(projectMHandle,projectm_get_hard_cut_enabled(projectMHandle));
}

void myRenderer::prevPreset(bool prev) {
	if (prev)
		projectm_select_previous_preset(projectMHandle,projectm_get_hard_cut_enabled(projectMHandle));
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
	settings->window_width = 150;
	settings->window_height = 150;
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
	oldIndex=-1;
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
	if (module)
		renderer.init(renderer.initSettings());
}

Display::~Display() {

}

void Display::step() {
	if (module) {
		renderer.handleWindowSize();
		renderer.handlePreset();
		renderer.handlePCMData(module->pcmData);
		renderer.nextPreset(module->next);
		renderer.prevPreset(module->prev);
		renderer.locked = module->locked;
		renderer.hard_cut = module->hard_cut;
		renderer.index=module->index;
		module->presetName = renderer.getName();
		module->presetSize = renderer.getPresets().size();
	}
	// Render every frame
	dirty = true;

	OpenGlWidget::step();
}

void Display::drawFramebuffer() {
	//renderer.draw();
	if (renderer.projectMHandle)
		projectm_render_frame(renderer.projectMHandle);
}

RPJVisualizer::~RPJVisualizer() {

}

RPJVisualizer::RPJVisualizer() {

	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

	configSwitch(PARAM_NEXT, 0.f, 1.f, 1.f, "Next preset");
	configSwitch(PARAM_PREV, 0.f, 1.f, 1.f, "Previous preset");
	configSwitch(PARAM_LOCK, 0.f, 1.f, 1.f, "Preset lock mode", {"Locked", "Unlocked"});
	configSwitch(PARAM_HARD_CUT, 0.f, 1.f, 1.f, "Hard cut mode", {"Enabled", "Disabled"});
	configButton(PARAM_NEXT, "Next preset");
	configButton(PARAM_PREV, "Previous preset");
	locked = false;
	lightDivider.setDivision(16);

	// Returns a list of all presets currently loaded by projectM
	next=false;
	prev=false;
	index=-1;
}

void RPJVisualizer::process(const ProcessArgs &args) {

	pcmData[0] = inputs[RPJVisualizer::INPUT_INL].getVoltage();
	pcmData[1] = inputs[RPJVisualizer::INPUT_INR].getVoltage();
	
	locked = params[PARAM_LOCK].getValue() <= 0.f;
	
	hard_cut = params[PARAM_HARD_CUT].getValue() <= 0.f;
	
	if (nextTrigger.process(params[PARAM_NEXT].getValue() > 0.f)) {
		next=true;
	}
	if (params[PARAM_NEXT].getValue() == 0.f)
		next=false;

	if (prevTrigger.process(params[PARAM_PREV].getValue() > 0.f)) {
		prev=true;
	}
	if (params[PARAM_PREV].getValue() == 0.f)
		prev=false;

	if (lightDivider.process()) {
		lights[LOCK_LIGHT].setBrightness(locked);
		lights[HARD_CUT_LIGHT].setBrightness(hard_cut);
	}

	lights[NEXT_LIGHT].setBrightness(next);
	lights[PREV_LIGHT].setBrightness(prev);
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
	Display *display;
	VisualizerModuleWidget(RPJVisualizer* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		box.size = Vec(MODULE_WIDTH*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		
		display = new Display();
		display->box.pos = Vec(80, 5);
		display->box.size = Vec(WIDTH, HEIGHT);
		display->module = module;
		addChild(display);

		
		//PresetNameDisplay * pnd = new PresetNameDisplay(Vec(29,130));
		//pnd->module = module;
		//addChild(pnd);
		
		// Then do the knobs
		const float knobX1 = 27;

		const float knobY1 = 33;
		const float knobY2 = 79;
		//const float knobY3 = 122;
		//const float knobY4 = 150;
		//const float knobY5 = 178;
		//const float knobY6 = 206;

		const float buttonX1 = 41;

		const float buttonY1 = 185;
		const float buttonY2 = 215;
		const float buttonY3 = 245;
		const float buttonY4 = 275;

		const float jackX1 = 11;
		const float jackX2 = 47;

		const float jackY1 = 311;

		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, RPJVisualizer::PARAM_NEXT,RPJVisualizer::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, RPJVisualizer::PARAM_PREV,RPJVisualizer::PREV_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY3), module, RPJVisualizer::PARAM_HARD_CUT, RPJVisualizer::HARD_CUT_LIGHT));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY4), module, RPJVisualizer::PARAM_LOCK, RPJVisualizer::LOCK_LIGHT));
		
		addInput(createInput<PJ301MPort>(Vec(jackX1, jackY1), module, RPJVisualizer::INPUT_INL));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY1), module, RPJVisualizer::INPUT_INR));	
	}

	void appendContextMenu(Menu *menu) override {
		RPJVisualizer *module = dynamic_cast<RPJVisualizer*>(this->module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createIndexPtrSubmenuItem("PlayList", display->renderer.getPresets(), &module->index));

	}
};

Model * modelVisualizer = createModel<RPJVisualizer, VisualizerModuleWidget>("RPJVisualizer");
