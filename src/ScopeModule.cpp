#define NANOVG_GL3
#include "RPJ.hpp"
#include "nanovg_gl.h"
#include "ctrl/RPJKnobs.hpp"
#include "ctrl/RPJButtons.hpp"
#include <thread>

// Then do the knobs
const float knobX1 = 12;
const float knobX2 = 48;

const float knobY1 = 32;
const float knobY2 = 72;
const float knobY3 = 121;
const float knobY4 = 169;
const float knobY5 = 220;

const float buttonX1 = 26;

const float buttonY1 = 278;
const float buttonY2 = 306;

const float jackX1 = 14;
const float jackX2 = 50;

const float jackY1 = 74;
const float jackY2 = 123;
const float jackY3 = 172;
const float jackY4 = 221;
const float jackY5 = 266;
const float jackY6 = 295;
const float jackY7 = 327;

bool alwaysOnTop = false;
bool noFrames = false;
int windowedXpos = 100;
int windowedYpos = 100;
float windowedWidth = 640;
float windowedHeight = 480;

struct ScopeModule : Module {
  enum ParamIds {
    PARAM_NEXT,
		PARAM_PREV,
    PARAM_TIMER,
    PARAM_BEAT_SENS,
    PARAM_HARD_SENS,
    PARAM_HARD_DURATION,
    PARAM_GRADIENT,
    PARAM_PRESETTYPE,
    PARAM_ADDFAV,
    PARAM_DELFAV,
    NUM_PARAMS
  };
  enum InputIds {
    LEFT_INPUT, 
    RIGHT_INPUT,
    NEXT_PRESET_INPUT,
    PREV_PRESET_INPUT,
    BEAT_INPUT,
    HARDCUT_INPUT,
    HARDCUT_DURATION_INPUT,
    GRADIENT_INPUT,
    NUM_INPUTS
  };
  enum OutputIds {
    NUM_OUTPUTS
  };
  enum LightIds {
    NEXT_LIGHT,
		PREV_LIGHT,
    NUM_LIGHTS
  };

  ScopeModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configButton(PARAM_PRESETTYPE, "PlayList Activation");   
    configButton(PARAM_ADDFAV, "Add Visual");
	  configButton(PARAM_DELFAV, "Remove Visual");   
    configButton(PARAM_NEXT, "Next preset");
	  configButton(PARAM_PREV, "Previous preset");
    configParam(PARAM_TIMER, 0.f, 300.f, 30.f, "Time till next preset"," Seconds");
    configParam(PARAM_BEAT_SENS, 0.f, 5.f, 1.f, "Beat sensitivity","");
    configParam(PARAM_HARD_SENS, 0.f, 5.f, 1.f, "Hardcut sensitivity","");
    configParam(PARAM_HARD_DURATION, 0.f, 300.f, 30.f, "Hardcut duration"," Seconds");
    configParam(PARAM_GRADIENT, 0.f, 5.f, 1.f, "Gradient"," ");
  }

  void process(const ProcessArgs& args) override {
    
  }

  json_t *dataToJson() override {
	json_t *rootJ=json_object();

    json_object_set_new(rootJ, "AlwaysOnTop", json_boolean(alwaysOnTop));
    json_object_set_new(rootJ, "NoFrames", json_boolean(noFrames));

      json_object_set_new(rootJ, "windowedXpos", json_integer(windowedXpos));
      json_object_set_new(rootJ, "windowedYpos", json_integer(windowedYpos));
      json_object_set_new(rootJ, "windowedWidth", json_integer(windowedWidth));
      json_object_set_new(rootJ, "windowedHeight", json_integer(windowedHeight));

	  return rootJ;
  }

  void dataFromJson(json_t *rootJ) override {
    json_t *nAlwaysOnTopJ = json_object_get(rootJ, "AlwaysOnTop");
    json_t *nNoFramesJ = json_object_get(rootJ, "NoFrames");
    json_t *nWindowedXposJ = json_object_get(rootJ, "windowedXpos");
    json_t *nWindowedYposJ = json_object_get(rootJ, "windowedYpos");
    json_t *nWindowedWidthJ = json_object_get(rootJ, "windowedWidth");
    json_t *nWindowedHeightJ = json_object_get(rootJ, "windowedHeight");
    if (nWindowedWidthJ) {
	      windowedWidth = json_integer_value(nWindowedWidthJ);
    }
    if (nWindowedHeightJ) {
	      windowedHeight = json_integer_value(nWindowedHeightJ);
    }
    if (nWindowedXposJ) {
	      windowedXpos = json_integer_value(nWindowedXposJ);
    }
    if (nWindowedYposJ) {
	      windowedYpos = json_integer_value(nWindowedYposJ);
    }
    if (nAlwaysOnTopJ) {
	    alwaysOnTop = json_boolean_value(nAlwaysOnTopJ);
    }
    if (nNoFramesJ) {
	    noFrames = json_boolean_value(nNoFramesJ);
    }
  }
};

struct Display : OpenGlWidget {
  
  GLFWwindow* c;

  ScopeModule* module;

  Display(Vec size,bool alwaysOnTop,bool noFrames) {
    
      glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
      glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
      glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
      c = glfwCreateWindow(windowedWidth,windowedHeight, "External Scope", NULL, NULL);
      if (!c) 
        return;
    
      glfwSetWindowUserPointer(c, this);
      glfwSetWindowCloseCallback(c, [](GLFWwindow* w) { glfwIconifyWindow(w); });
  }

  void drawFramebuffer() override {
    int width,height;
    GLFWwindow* win = glfwGetCurrentContext();
    glfwMakeContextCurrent(nullptr);
    glfwMakeContextCurrent(c);
    glfwGetFramebufferSize(c,&width,&height);
    glViewport(0.0, 0.0, width, height);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, width, 0.0, height, -1.0, 1.0);

    glBegin(GL_TRIANGLES);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(width, 0, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, height, 0);
    glEnd();
    glfwSwapBuffers(c);
    glfwMakeContextCurrent(win);
  }

  void onRemove(const RemoveEvent &e) override {
    OpenGlWidget::onRemove(e);
  }

  void step() override {
    dirty = true;
	  FramebufferWidget::step();
  }
};

struct ScopeModuleWidget : ModuleWidget {
  GLFWwindow* c;
  Display* display;

    ScopeModuleWidget(ScopeModule* module) {
    
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VisualizerWindow.svg")));
//setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ModularForecast.svg")));
    addParam(createParam<ButtonBig>(Vec(17,30),module, ScopeModule::PARAM_PRESETTYPE));
    addParam(createParam<ButtonPlusBig>(Vec(7,45),module, ScopeModule::PARAM_ADDFAV));
    addParam(createParam<ButtonMinBig>(Vec(25,45),module, ScopeModule::PARAM_DELFAV));     
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, ScopeModule::PARAM_TIMER));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY2), module, ScopeModule::PARAM_BEAT_SENS));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY3), module, ScopeModule::PARAM_HARD_SENS));

    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, ScopeModule::PARAM_NEXT,ScopeModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, ScopeModule::PARAM_PREV,ScopeModule::PREV_LIGHT));

   	addInput(createInput<PJ301MPort>(Vec(jackX2, jackY1), module, ScopeModule::BEAT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY2), module, ScopeModule::HARDCUT_INPUT));	 

    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY5), module, ScopeModule::NEXT_PRESET_INPUT));
    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY6), module, ScopeModule::PREV_PRESET_INPUT));
    
    addInput(createInput<PJ301MPort>(Vec(jackX1, jackY7), module, ScopeModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY7), module, ScopeModule::RIGHT_INPUT));

    addParam(createParam<RPJKnob>(Vec(knobX1,knobY4), module, ScopeModule::PARAM_HARD_DURATION));	

    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY3), module, ScopeModule::HARDCUT_DURATION_INPUT));

    if (module) {
      GLFWwindow* win = glfwGetCurrentContext();
      glfwMakeContextCurrent(nullptr);
      display = new Display(Vec(windowedWidth, windowedHeight),alwaysOnTop,noFrames);
      display->module = module;
      addChild(display);
      glfwMakeContextCurrent(nullptr);
      glfwMakeContextCurrent(win);
    }
  }

  void step() override {
    display->drawFramebuffer();
    ModuleWidget::step();
  }
};


Model *modelScopeModule = createModel<ScopeModule, ScopeModuleWidget>("ExternalScope");