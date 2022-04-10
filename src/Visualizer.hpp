#include "rack.hpp"
#include <thread>
#include "../dep/include/libprojectM/projectM.h"

using namespace rack;

const int MODULE_WIDTH=31;
const int WIDTH=380;
const int HEIGHT=380;
const int PRESETDISPLAYLENGTH=12;

struct Display;

struct parameters {
	
};

struct myRenderer {
	myRenderer();
	~myRenderer();
	//void init(projectm_handle);
	projectm_handle initSettings();
	void Initialize(projectm_settings *);
	std::vector<std::string> getPresets();
	void handleWindowSize(float,float,float);
	void handlePreset(int,int,bool);
	void handleLocked(bool);
	std::string getNamePreset(projectm_handle);
	std::string getName();
	void draw();
	void nextPreset(bool,bool);
	void prevPreset(bool,bool);
	void handlePCMData(float *);
	void handleHardCut(bool);
	projectm_handle projectMHandle;
	float prevZoomlevel,prevRx,prevRy,prevWidth;
	size_t renderWidth;
	size_t renderHeight;
	int index;
	bool locked;
	bool hard_cut;
	std::thread renderThread;
	//GLFWwindow* window;
	bool changed=false;
	bool changeProcessed=false;
};

struct RPJVisualizer : Module {

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
		RPJVisualizer();
		~RPJVisualizer();
		void process(const ProcessArgs &) override;
		void processChannel(Input&, Input&, Output&);
		json_t *dataToJson() override;
		void dataFromJson(json_t *) override;
		bool shuffleEnabled;
		dsp::BooleanTrigger nextBoolTrigger,prevBoolTrigger;
		dsp::SchmittTrigger nextSchmittTrigger,prevSchmittTrigger;
		bool locked;
		bool hard_cut;
		dsp::ClockDivider lightDivider;
		int rating_list[3] = {1,2,3};
		unsigned int currentIndex;
		bool next,prev,nextLight,prevLight;
		float pcmData[2];
		int index;
		std::string presetName;
		int presetSize;
		float resizeX,resizeY;
		float posX,posY;
		int timer;
		float width = WIDTH;
};

struct Display : OpenGlWidget {
	Display();
	~Display();
	void drawFramebuffer() override;
	void step() override;
	ModuleWidget *widget;
	myRenderer renderer;
	RPJVisualizer *module;
	int oldIndex;
};
