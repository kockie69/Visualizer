#include "rack.hpp"
#include "../dep/include/libprojectM/projectM.h"

using namespace rack;

const int MODULE_WIDTH=31;
const int WIDTH=370;
const int HEIGHT=370;

struct RPJVisualizer : Module {

	enum ParamIds {
		PARAM_DURATION,
		PARAM_LOCK,
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
		LOCK_LIGHT,
		NUM_LIGHTS,
	};
		RPJVisualizer();
		void process(const ProcessArgs &) override;
		void processChannel(Input&, Input&, Output&);
		json_t *dataToJson() override;
		void dataFromJson(json_t *) override;
		float pcmData[2];
		projectm_settings *settings;
		projectm_handle projectMHandle;
		bool locked;
		dsp::ClockDivider lightDivider;
		int index;
		bool hard_cut;
		int rating_list[3] = {1,2,3};
		unsigned int currentIndex;
		std::vector<std::string> presets;
};

struct Display : OpenGlWidget {
	Display();
	~Display();
	void drawFramebuffer() override;
	void step() override;

	RPJVisualizer *module;
	float prevZoomlevel;
	projectm *pm;
	size_t renderWidth;
	size_t renderHeight;
};
