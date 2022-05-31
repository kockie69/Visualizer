#define NANOVG_GL2
#include "RPJ.hpp"
#include "dsp/digital.hpp"
#include "nanovg_gl.h"
#include "../dep/include/libprojectM/projectM.h"
#include "Renderer.hpp"
#include "linmath.h"
#include "stb_image.h"
#include "ctrl/RPJKnobs.hpp"
#include "JWResizableHandle.hpp"

#include <thread>

static const unsigned int kSampleWindow = 1;

// Then do the knobs
const float knobX1 = 11;
const float knobX2 = 27;
const float knobX3 = 47;

const float knobY1 = 44;
const float knobY2 = 311;
//const float knobY3 = 122;
//const float knobY4 = 150;
//const float knobY5 = 178;
//const float knobY6 = 206;

const float buttonX1 = 41;

const float buttonY0 = 100;
const float buttonY1 = 185;
const float buttonY2 = 215;
const float buttonY3 = 245;
const float buttonY4 = 275;

struct MilkrackModule : Module {
  enum ParamIds {
    PARAM_NEXT,
		PARAM_PREV,
    PARAM_HARD_CUT,
    PARAM_TIMER,
    NUM_PARAMS
  };
  enum InputIds {
    LEFT_INPUT, 
    RIGHT_INPUT,
    NEXT_PRESET_INPUT,
    NUM_INPUTS
  };
  enum OutputIds {
    NUM_OUTPUTS
  };
  enum LightIds {
    NEXT_LIGHT,
		PREV_LIGHT,
    HARD_CUT_LIGHT,
    NUM_LIGHTS
  };

  MilkrackModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configButton(PARAM_NEXT, "Next preset");
	  configButton(PARAM_PREV, "Previous preset");
    configSwitch(PARAM_HARD_CUT, 0.f, 1.f, 1.f, "Hard cut mode", {"Enabled", "Disabled"});
    configParam(PARAM_TIMER, 0.f, 300.f, 30.f, "Time till next preset","Seconds");
    lightDivider.setDivision(16);
  }
  int presetIndex = 0;
  unsigned int i = 0;
  bool full = false;
  bool nextPreset = false;
  bool prevPreset = false;
  bool changeHardcut = false;
  bool hard_cut = false;
  dsp::SchmittTrigger hardcutTrigger;
  dsp::BooleanTrigger nextTrigger,prevTrigger;
  dsp::ClockDivider lightDivider;
  float pcm_data[kSampleWindow];

  void step() override {
    
    pcm_data[i++] = inputs[LEFT_INPUT].value;
    if (inputs[RIGHT_INPUT].active)
      pcm_data[i++] = inputs[RIGHT_INPUT].value;
    else
      pcm_data[i++] = inputs[LEFT_INPUT].value;
    if (i >= kSampleWindow) {
      i = 0;
      full = true;
    }

    if (hardcutTrigger.process(rescale(params[PARAM_HARD_CUT].getValue(), 0.1f, 2.f, 0.f, 1.f))) {
		  changeHardcut=true;
	  }

    if (hardcutTrigger.process(rescale(params[PARAM_HARD_CUT].getValue(), 2.f, 0.1f, 0.f, 1.f))) {
		  changeHardcut=true;
	  }
    
    if (nextTrigger.process(params[PARAM_NEXT].getValue()) > 0.f) {
      nextPreset=true;
	  }

    if (prevTrigger.process(params[PARAM_PREV].getValue()) > 0.f) {
		  prevPreset=true;
	  }
    
    lights[NEXT_LIGHT].setBrightness(nextPreset);
	  lights[PREV_LIGHT].setBrightness(prevPreset);

    if (lightDivider.process()) {
      hard_cut = params[PARAM_HARD_CUT].getValue();
		  lights[HARD_CUT_LIGHT].setBrightness(hard_cut);
	  }
  }

  json_t *dataToJson() override {
	  json_t *rootJ=json_object();
	  json_object_set_new(rootJ, "ActivePreset", json_integer(presetIndex));
	  return rootJ;
  }

  void dataFromJson(json_t *rootJ) override {
	  json_t *nActivePresetJ = json_object_get(rootJ, "ActivePreset");
	  if (nActivePresetJ) {
	    presetIndex = json_integer_value(nActivePresetJ);
    }
  }
};


struct BaseProjectMWidget : FramebufferWidget {

  const int fps = 60;
  const bool debug = true;
  bool displayPresetName = false;

  MilkrackModule* module;

  BaseProjectMWidget() {}

  void init(std::string presetURL,int presetIndex) {
      getRenderer()->init(initSettings(presetURL,presetIndex));
  }

  template<typename T>
  static BaseProjectMWidget* create(Vec pos, std::string presetURL,int presetIndex) {
    BaseProjectMWidget* p = new T;
    p->box.pos = pos;
    p->init(presetURL,presetIndex);
    return p;
  }

  virtual ProjectMRenderer* getRenderer() = 0;

  void step() override {
    dirty = true;
    if (module) {
      
      module->presetIndex = getRenderer()->activePreset();

      if (module->full) {
        getRenderer()->addPCMData(module->pcm_data, kSampleWindow);
        module->full = false;
      }
      if (module->changeHardcut) {
        module->changeHardcut = false;
        getRenderer()->requestToggleHardcut();
      }
     // If the module requests that we change the preset at random
     // (i.e. the random button was clicked), tell the render thread to
     // do so on the next pass.
      if (module->nextPreset) {
        module->nextPreset = false;
        if (!getRenderer()->isAutoplayEnabled())
          //getRenderer()->selectNextPreset(getRenderer()->isHardcutEnabled());
          getRenderer()->selectNextPreset(true);
        else 
          getRenderer()->requestPresetID(kPresetIDRandom);
      }
      if (module->prevPreset) {
        module->prevPreset = false;
        if (!getRenderer()->isAutoplayEnabled())
          //getRenderer()->selectPreviousPreset(getRenderer()->isHardcutEnabled());
          getRenderer()->selectPreviousPreset(true);
        else
          getRenderer()->requestPresetID(kPresetIDRandom);
      }
    }
  }

  void randomize() {
    // Tell the render thread to switch to another preset on the next
    // pass.
    getRenderer()->requestPresetID(kPresetIDRandom);
  }

  // Builds a Settings object to initialize the member one in the
  // object.  This is needed because we must initialize this->settings
  // before starting this->renderThread, and that has to be done in
  // ProjectMWidget's ctor init list.
  mySettings initSettings(std::string presetURL,int presetIndex) const {
    mySettings s;

    // Window/rendering settings
    s.presetIndex = presetIndex;
    s.preset_url = (char *)presetURL.c_str();
    s.window_width = 360;
    s.window_height = 360;
    s.fps =  60;
    s.mesh_x = 220;
    s.mesh_y = 125;
    s.aspect_correction = true;

    // Preset display settings
    s.preset_duration = 30;
    s.soft_cut_duration = 10;
    s.hard_cut_enabled = false;
    s.hard_cut_duration= 20;
    s.hard_cut_sensitivity =  1.0;
    s.beat_sensitivity = 1.0;
    s.shuffle_enabled = false;

    // Unsupported settings
    //s.softCutRatingsEnabled = false;
    //s.menuFontURL = nullptr;
    //s.titleFontURL = nullptr;
    return s;
  }
};

struct WindowedProjectMWidget : BaseProjectMWidget {
  WindowedRenderer* renderer;

  WindowedProjectMWidget() : renderer(new WindowedRenderer) {}

  ProjectMRenderer* getRenderer() override { return renderer; }

  void draw(NVGcontext* vg) override {
    std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
    nvgSave(vg);
    nvgBeginPath(vg);
    nvgFillColor(vg, nvgRGB(0xff, 0xff, 0xff));
    nvgFontSize(vg, 14);
    nvgFontFaceId(vg, font->handle);
    nvgTextAlign(vg, NVG_ALIGN_BOTTOM);
    nvgScissor(vg, 5, 5, 20, 330);
    nvgRotate(vg, M_PI/2);
    if (!getRenderer()->isRendering()) {
      nvgText(vg, 5, -5, "Unable to initialize rendering. See log for details.", nullptr);
    } else {
      nvgText(vg, 5, -5, getRenderer()->activePresetName().c_str(), nullptr);
    }
    nvgFill(vg);
    nvgClosePath(vg);
    nvgRestore(vg);
  }
};

struct EmbeddedProjectMWidget : BaseProjectMWidget {
  int img;
  unsigned int shaderProgram;
  
  TextureRenderer* renderer;

  EmbeddedProjectMWidget() : renderer(new TextureRenderer) {
  }

  ProjectMRenderer* getRenderer() override { return renderer; }

  void draw(const DrawArgs &args) override {
    const int y = 360;
    int x = renderer->getWindowWidth();

    nvgDeleteImage(args.vg,img);
    img = nvgCreateImageRGBA(args.vg,x,y,0,renderer->getBuffer());
    std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
 
    NVGpaint imgPaint = nvgImagePattern(args.vg, 0, 0, renderer->getWindowWidth(), y, 0.0f, img, 1.0f);

    nvgSave(args.vg);
    nvgScale(args.vg, 1, -1); // flip
    nvgTranslate(args.vg,0, -y);
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, x, y); 
    nvgFillPaint(args.vg, imgPaint);
    nvgFill(args.vg);
    nvgRestore(args.vg);

    if (displayPresetName) {
      nvgSave(args.vg);
      nvgScissor(args.vg, 0, 0, x, y);
      nvgBeginPath(args.vg);
      nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
      nvgFontSize(args.vg, 14);
      nvgFontFaceId(args.vg, font->handle);
      nvgTextAlign(args.vg, NVG_ALIGN_BOTTOM);
      nvgText(args.vg, 10, 20, getRenderer()->activePresetName().c_str(), nullptr);
      nvgFill(args.vg);
      nvgClosePath(args.vg);
      nvgRestore(args.vg);
    }
    FramebufferWidget::draw(args);
  }
};

struct SetPresetMenuItem : MenuItem {
  BaseProjectMWidget* w;
  unsigned int presetId;

  void onAction(const ActionEvent& e) override {
    w->getRenderer()->requestPresetID(presetId);
  }

  void step() override {
    rightText = (w->getRenderer()->activePreset() == presetId) ? "<<" : "";
    MenuItem::step();
  }

  static SetPresetMenuItem* construct(std::string label, unsigned int i, BaseProjectMWidget* w) {
    SetPresetMenuItem* m = new SetPresetMenuItem;
    m->w = w;
    m->presetId = i;
    m->text = label;
    return m;
  }
};

struct ToggleAutoplayMenuItem : MenuItem {
  BaseProjectMWidget* w;

  void onAction(const ActionEvent& e) override {
    w->getRenderer()->requestToggleAutoplay();
  }

  void step() override {
    rightText = (w->getRenderer()->isAutoplayEnabled() ? "yes" : "no");
    MenuItem::step();
  }

  static ToggleAutoplayMenuItem* construct(std::string label, BaseProjectMWidget* w) {
    ToggleAutoplayMenuItem* m = new ToggleAutoplayMenuItem;
    m->w = w;
    m->text = label;
    return m;
  }
};

struct ToggleDisplayPresetNameMenuItem : MenuItem {
  BaseProjectMWidget* w;

  void onAction(const ActionEvent& e) override {
    w->displayPresetName = !w->displayPresetName;
  }

  void step() override {
    rightText = (w->displayPresetName ? "yes" : "no");
    MenuItem::step();
  }

  static ToggleDisplayPresetNameMenuItem* construct(std::string label, BaseProjectMWidget* w) {
    ToggleDisplayPresetNameMenuItem* m = new ToggleDisplayPresetNameMenuItem;
    m->w = w;
    m->text = label;
    return m;
  }
};

struct BaseMilkrackModuleWidget : ModuleWidget {
 BaseProjectMWidget* w;

 // using ModuleWidget::ModuleWidget;

  //void randomizeAction() override {
  //  w->randomize();
  //}

  void appendContextMenu(Menu* menu) override {
    MilkrackModule* m = dynamic_cast<MilkrackModule*>(module);
    assert(m);

    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Options"));
    menu->addChild(ToggleAutoplayMenuItem::construct("Cycle through presets", w));
    if (m->getModel()->name == "MilkrackEmbedded" )
      menu->addChild(ToggleDisplayPresetNameMenuItem::construct("Show Preset Title", w));

    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Preset"));
    auto presets = w->getRenderer()->listPresets();
    for (auto p : presets) {
      menu->addChild(SetPresetMenuItem::construct(p.second, p.first, w));
    }
  }
};

struct MilkrackModuleWidget : BaseMilkrackModuleWidget {

  MilkrackModuleWidget(MilkrackModule* module) {

  setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VisualizerWindow.svg")));
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, MilkrackModule::PARAM_TIMER));
    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, MilkrackModule::PARAM_NEXT,MilkrackModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, MilkrackModule::PARAM_PREV,MilkrackModule::PREV_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY0), module, MilkrackModule::PARAM_HARD_CUT, MilkrackModule::HARD_CUT_LIGHT));

		addInput(createInput<PJ301MPort>(Vec(knobX1, knobY2), module, MilkrackModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(knobX3, knobY2), module, MilkrackModule::RIGHT_INPUT));	

    addInput(createInput<PJ301MPort>(Vec(30, 240), module, MilkrackModule::NEXT_PRESET_INPUT));

    //std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
    if (module) {
      w = BaseProjectMWidget::create<WindowedProjectMWidget>(Vec(85, 20), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex);
      w->module = module;
      //w->font = font;
      addChild(w);
    }
  }
};

struct EmbeddedMilkrackModuleWidget : BaseMilkrackModuleWidget {
    JWModuleResizeHandle *rightHandle;
    BGPanel *panel;
    EmbeddedMilkrackModuleWidget(MilkrackModule* module) {

    setModule(module);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

		panel = new BGPanel(nvgRGB(0, 0, 0));
		panel->box.size = box.size;
		int x = box.size.x;
		int y = box.size.y;
		addChild(panel);

    if (module) {
      w = BaseProjectMWidget::create<EmbeddedProjectMWidget>(Vec(85, 0), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex);
      w->module = module;
      w->box.size = Vec(360,360);
      addChild(w);

      JWModuleResizeHandle *rightHandle = new JWModuleResizeHandle(w->getRenderer()->window);
		  rightHandle->right = true;
		  this->rightHandle = rightHandle;

		  addChild(rightHandle);
    }

        
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, MilkrackModule::PARAM_TIMER));
    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, MilkrackModule::PARAM_NEXT,MilkrackModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, MilkrackModule::PARAM_PREV,MilkrackModule::PREV_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY0), module, MilkrackModule::PARAM_HARD_CUT, MilkrackModule::HARD_CUT_LIGHT));

		addInput(createInput<PJ301MPort>(Vec(knobX1, knobY2), module, MilkrackModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(knobX3, knobY2), module, MilkrackModule::RIGHT_INPUT));	

    addInput(createInput<PJ301MPort>(Vec(30, 240), module, MilkrackModule::NEXT_PRESET_INPUT));
  }

  void step() override {
		panel->box.size = box.size;
    if (module) {
		  rightHandle->box.pos.x = box.size.x - rightHandle->box.size.x;
      w->box.size = rightHandle->box.size;
    }
    ModuleWidget::step();
  }
};

Model *modelWindowedMilkrackModule = createModel<MilkrackModule, MilkrackModuleWidget>("MilkrackFull");
Model *modelEmbeddedMilkrackModule = createModel<MilkrackModule, EmbeddedMilkrackModuleWidget>("MilkrackEmbedded");