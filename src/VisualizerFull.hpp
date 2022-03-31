#include "rack.hpp"
#include "../dep/include/libprojectM/projectM.h"
#include <thread>

using namespace rack;

const int MODULE_WIDTH=26;
const int WIDTH=360;
const int HEIGHT=360;

struct RPJVisualizerFull : Module {

	enum ParamIds {
		PARAM_DURATION,
		NUM_PARAMS,
	};

	enum InputIds {
		INPUT_INL,
		INPUT_INR,
		NUM_INPUTS,
	};

	enum OutputIds {
		NUM_OUTPUTS,
	};

	enum LightsIds {
		NUM_LIGHTS,
	};
		RPJVisualizerFull();
		void process(const ProcessArgs &) override;
		void processChannel(Input&, Input&, Output&);
		float pcmData[2];
		projectm_settings *settings;
		projectm_handle projectMHandle;
		size_t renderWidth;
		size_t renderHeight;
		float prevZoomlevel;
};

struct Renderer {
	GLFWwindow* createWindow();
	RendererFull();
	void process(projectm_handle, GLFWwindow*);
};

struct DisplayFull : FramebufferWidget {
	DisplayFull();
	void init();
	void drawFramebuffer() override;
	void step() override;
	//projectm *pm;
	RPJVisualizerFull *module;
	Renderer *renderer;
	GLFWwindow *myWindow;
	std::thread renderThread;
};