#include "RPJ.hpp"
#include "VisualizerFull.hpp"
#include "ctrl/RPJKnobs.hpp"


DisplayFull::DisplayFull() {

}

void DisplayFull::init() {
	if (!module)
		return;
	projectm_settings *s = module->settings;	
	renderer = new Renderer();
	myWindow = renderer->createWindow();
	renderThread = std::thread([this, s](){ this->drawFramebuffer(); });
}

void DisplayFull::step() {
	// Render every frame
	dirty = true;

	FramebufferWidget::step();
}

GLFWwindow* Renderer::createWindow() {
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  	glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	GLFWwindow* c = glfwCreateWindow(360, 360, "", NULL, NULL); 
	//GLFWwindow* c = glfwCreateWindow(WIDTH, HEIGHT, "My Title", glfwGetPrimaryMonitor(), NULL); 
	if (!c) {
		//rack::loggerLog(rack::DEBUG_LEVEL, "Milkrack/" __FILE__, __LINE__, "Milkrack renderLoop could not create a context, bailing.");
		return nullptr;
	}
	glfwSetWindowUserPointer(c, reinterpret_cast<void*>(this));
	//glfwSetFramebufferSizeCallback(c, framebufferSizeCallback);
	glfwSetWindowCloseCallback(c, [](GLFWwindow* w) { glfwIconifyWindow(w); });
	//glfwSetKeyCallback(c, keyCallback);
	glfwSetWindowTitle(c, u8"Marbles");
	return c;
}

void Renderer::process(projectm_handle handle, GLFWwindow *myWindow) {
	if (!handle)
		return;
	else {
		projectm_render_frame(handle);
	}
}

void DisplayFull::drawFramebuffer() {
	if (!module->projectMHandle)
	{
    	return;
	}
	else {
		projectm_pcm_add_float_2ch_data(module->projectMHandle,module->pcmData,2);
		projectm_render_frame(module->projectMHandle);
	}
}

RPJVisualizerFull::RPJVisualizerFull() {

	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	configParam(PARAM_DURATION, 1.f, 100.f,10.f, "Preset duration playing");

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
	settings->hard_cut_enabled = false;
	settings->hard_cut_duration = 20;
	settings->hard_cut_sensitivity =  1.0;
	settings->beat_sensitivity = 1.0;
	settings->shuffle_enabled = true;
	std::string thePath = asset::plugin(pluginInstance, "presets/");
	settings->preset_url = (char *)thePath.c_str();
	// Unsupported settings
	settings->soft_cut_ratings_enabled = false;
	settings->menu_font_url = nullptr;
	settings->title_font_url = nullptr;

	prevZoomlevel= APP->scene->rackScroll->getZoom();

	projectMHandle = projectm_create_settings(settings, PROJECTM_FLAG_NONE);
	renderWidth = WIDTH;
	renderHeight = HEIGHT;

	if (!projectMHandle)
		return;
	else
		projectm_set_window_size(projectMHandle, WIDTH, HEIGHT);
}


void RPJVisualizerFull::process(const ProcessArgs &args) {
	pcmData[0] = inputs[RPJVisualizerFull::INPUT_INL].getVoltage();
	pcmData[1] = inputs[RPJVisualizerFull::INPUT_INR].getVoltage();
	projectm_set_preset_duration(projectMHandle,params[PARAM_DURATION].getValue());
}

struct VisualizerModuleWidgetFull : ModuleWidget {
	VisualizerModuleWidgetFull(RPJVisualizerFull* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));

		box.size = Vec(MODULE_WIDTH*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		
		DisplayFull *display = new DisplayFull();
		display->box.pos = Vec(83, 5);
		display->box.size = Vec(WIDTH, HEIGHT);
		display->module = module;
		display->init();
		addChild(display);

		// Then do the knobs
		//const float knobX1 = 16;
		const float knobX2 = 32;
		//const float knobX3 = 52;

		const float knobY1 = 33;
		//const float knobY2 = 80;
		//const float knobY3 = 122;
		//const float knobY4 = 150;
		//const float knobY5 = 178;
		//const float knobY6 = 206;

		//addParam(createParam<RPJKnob>(Vec(knobX2, knobY1), module, RPJVisualizer::PARAM_DURATION));
		
		
		//addInput(createInput<PJ301MPort>(Vec(11, 320), module, RPJVisualizer::INPUT_INL));	
		//addInput(createInput<PJ301MPort>(Vec(11, 350), module, RPJVisualizer::INPUT_INR));	

	}
};

Model * modelVisualizerFull = createModel<RPJVisualizerFull, VisualizerModuleWidgetFull>("RPJVisualizerFull");