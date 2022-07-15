#define NANOVG_GL2
#include "RPJ.hpp"
#include "nanovg_gl.h"
#ifdef ARCH_WIN
#include "../dep/include/libprojectM/Windows/projectM.h"
#endif
#ifdef ARCH_LIN
#include "../dep/include/libprojectM/Linux/projectM.h"
#endif
#ifdef ARCH_MAC
#include "../dep/include/libprojectM/Mac/projectM.h"
#endif
#include "Renderer.hpp"
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

const float buttonX1 = 41;

const float buttonY0 = 100;
const float buttonY1 = 185;
const float buttonY2 = 215;
const float buttonY3 = 245;
const float buttonY4 = 275;

struct LFMModule : Module {
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

  LFMModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configButton(PARAM_NEXT, "Next preset");
	  configButton(PARAM_PREV, "Previous preset");
    configSwitch(PARAM_HARD_CUT, 0.f, 1.f, 1.f, "Hard cut mode", {"Enabled", "Disabled"});
    configParam(PARAM_TIMER, 0.f, 300.f, 30.f, "Time till next preset"," Seconds");
    lightDivider.setDivision(16);
  }
  float presetTime = 0;
  int presetIndex = 0;
  bool displayPresetName = false;
  bool autoPlay = false;
  unsigned int i = 0;
  bool full = false;
  bool nextPreset = false;
  bool prevPreset = false;
  bool hard_cut = false;
  dsp::SchmittTrigger hardcutTrigger,nextInputTrigger;
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

    presetTime = params[PARAM_TIMER].getValue();

    hard_cut = params[PARAM_HARD_CUT].getValue();
    
    if (nextTrigger.process(params[PARAM_NEXT].getValue()) > 0.f || nextInputTrigger.process(rescale(inputs[NEXT_PRESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
      nextPreset=true;
	  }

    if (prevTrigger.process(params[PARAM_PREV].getValue()) > 0.f) {
		  prevPreset=true;
	  }
    
    lights[NEXT_LIGHT].setBrightness(nextPreset);
	  lights[PREV_LIGHT].setBrightness(prevPreset);

    if (lightDivider.process()) {
		  lights[HARD_CUT_LIGHT].setBrightness(hard_cut);
	  }
  }

  json_t *dataToJson() override {
	  json_t *rootJ=json_object();
	  json_object_set_new(rootJ, "ActivePreset", json_integer(presetIndex));
    json_object_set_new(rootJ, "DisplayPresetName", json_boolean(displayPresetName));
    json_object_set_new(rootJ, "Autoplay", json_boolean(autoPlay));
	  return rootJ;
  }

  void dataFromJson(json_t *rootJ) override {
	  json_t *nActivePresetJ = json_object_get(rootJ, "ActivePreset");
    json_t *nDisplayPresetNameJ = json_object_get(rootJ, "DisplayPresetName");
    json_t *nAutoplayJ = json_object_get(rootJ, "Autoplay");
	  if (nActivePresetJ) {
	    presetIndex = json_integer_value(nActivePresetJ);
    }
    if (nDisplayPresetNameJ) {
	    displayPresetName = json_boolean_value(nDisplayPresetNameJ);
    }
    if (nAutoplayJ) {
	    autoPlay = json_boolean_value(nAutoplayJ);
    }
  }
};


struct BaseProjectMWidget : FramebufferWidget {

  const int fps = 60;
  const bool debug = true;
  bool oldAutoPlay = false;

  LFMModule* module;

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

  void onRemove(const RemoveEvent &e) override {
    delete getRenderer();
    FramebufferWidget::onRemove(e);
  }

  void step() override {
    dirty = true;
    if (module) {
      getRenderer()->presetTime = module->presetTime;
      module->presetIndex = getRenderer()->activePreset();
      if (module->autoPlay != getRenderer()->isAutoplayEnabled())
        getRenderer()->requestToggleAutoplay();
        
      if (module->full) {
        getRenderer()->addPCMData(module->pcm_data, kSampleWindow);
        module->full = false;
      }
      if (module->hard_cut != getRenderer()->isHardcutEnabled()) {
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
    
    const char * endday = "20220811";
    std::time_t t = std::time(0);   // get time now
    std::tm* now = std::localtime(&t);
    int year = now->tm_year + 1900;
    int mon = now->tm_mon+1;
    int day = now->tm_mday;
    std::string monstr = std::to_string(mon);
    if (mon < 10)
      monstr = "0"+monstr;
    std::string daystr = std::to_string(day);
    if (day < 10)
      daystr = "0"+daystr; 
    std::string today = std::to_string(year)+monstr+daystr;
    if (std::stoi(today)>std::stoi(endday))
      s.preset_url = (char *)"";
    else s.preset_url = (char *)presetURL.c_str();
    
    s.window_width = RACK_GRID_HEIGHT;
    s.window_height = RACK_GRID_HEIGHT;
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

  void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
      std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
      nvgSave(args.vg);
      nvgBeginPath(args.vg);
      nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
      nvgFontSize(args.vg, 14);
      nvgFontFaceId(args.vg, font->handle);
      nvgTextAlign(args.vg, NVG_ALIGN_BOTTOM);
      nvgScissor(args.vg, 5, 5, 20, 330);
      nvgRotate(args.vg, M_PI/2);
      if (!getRenderer()->isRendering()) {
        nvgText(args.vg, 5, -5, "Unable to initialize rendering. See log for details.", nullptr);
      } else {
        nvgText(args.vg, 5, -5, getRenderer()->activePresetName().c_str(), nullptr);
      }
      nvgFill(args.vg);
      nvgClosePath(args.vg);
      nvgRestore(args.vg);
    }
  }
};

struct EmbeddedProjectMWidget : BaseProjectMWidget {
  int img;
  unsigned int shaderProgram;
  
  TextureRenderer* renderer;

  EmbeddedProjectMWidget() : renderer(new TextureRenderer) {
  }

  ProjectMRenderer* getRenderer() override { return renderer; }


    void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
      const int y = RACK_GRID_HEIGHT;
      int x = renderer->getWindowWidth();

      nvgDeleteImage(args.vg,img);
      img = nvgCreateImageRGBA(args.vg,x,y,0,renderer->getBuffer());
      std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
  
      NVGpaint imgPaint = nvgImagePattern(args.vg, 0, 0, renderer->getWindowWidth()+15, y+31, 0.0f, img, 1.0f);

      nvgSave(args.vg);
      nvgScale(args.vg, 1, -1); // flip
      nvgTranslate(args.vg,0, -y);
      nvgBeginPath(args.vg);
      nvgRect(args.vg, 0, 0, x, y); 
      nvgFillPaint(args.vg, imgPaint);
      nvgFill(args.vg);
      nvgRestore(args.vg);

      if (module->displayPresetName) {
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
    }
  }
};

struct SetPresetMenuItem : MenuItem {
  BaseProjectMWidget* w;
  unsigned int presetId;
  TextField* textfield;

  void onAction(const ActionEvent& e) override {
    w->getRenderer()->requestPresetID(presetId);
  }

  void step() override {
    if (text.find(textfield->getText()) != std::string::npos)
      visible=true;
    else
      visible=false;
    rightText = (w->getRenderer()->activePreset() == presetId) ? "<<" : "";
    MenuItem::step();
  }

  static SetPresetMenuItem* construct(std::string label, unsigned int i, BaseProjectMWidget* w,TextField* t) {
    SetPresetMenuItem* m = new SetPresetMenuItem;

    m->w = w;
    m->presetId = i;
    m->text = label;
    m->textfield = t;

    return m;
  }
};

struct BaseLFMModuleWidget : ModuleWidget {
 BaseProjectMWidget* w;

 // using ModuleWidget::ModuleWidget;

  //void randomizeAction() override {
  //  w->randomize();
  //}

  void appendContextMenu(Menu* menu) override {
    LFMModule* m = dynamic_cast<LFMModule*>(module);
    assert(m);

    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Options"));
    menu->addChild(createBoolPtrMenuItem("Cycle through presets","", &m->autoPlay));
    if (m->getModel()->name == "LFMEmbedded" ) {
      DEBUG("Ok, embedded so we will add option to select a preset title");
      menu->addChild(createBoolPtrMenuItem("Show Preset Title","", &m->displayPresetName));
      DEBUG("Ok, we have added the option to select a preset title");
    }
    menu->addChild(construct<MenuLabel>());
    DEBUG("Ok, we now add the preset menu title");
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Visual Presets"));
    DEBUG("Ok, added menu title");
    
    DEBUG("Ok, we will now get the list of presets");
    auto holder = new rack::Widget;
    holder->box.size.x = 200;
    holder->box.size.y = 20;
    //auto lab = new rack::Label;
    //lab->text = "Search presets : ";
    //lab->box.size.x = 60;
    //holder->addChild(lab);
    auto textfield = new rack::TextField;
    textfield->box.pos.x = 0;
    textfield->box.size.x = 200;
    textfield->placeholder = "Search presets";
    holder->addChild(textfield);
    menu->addChild(holder);
    menu->addChild(construct<MenuLabel>());
    //auto presets = w->getRenderer()->listPresets();
    DEBUG("Ok, we have the list of presets");
    auto presets = w->getRenderer()->listPresets();
    for (auto p : presets) 
      menu->addChild(SetPresetMenuItem::construct(p.second, p.first, w,textfield));
  }
};

struct LFMModuleWidget : BaseLFMModuleWidget {

  LFMModuleWidget(LFMModule* module) {
    
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VisualizerWindow.svg")));
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, LFMModule::PARAM_TIMER));
    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, LFMModule::PARAM_NEXT,LFMModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, LFMModule::PARAM_PREV,LFMModule::PREV_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY0), module, LFMModule::PARAM_HARD_CUT, LFMModule::HARD_CUT_LIGHT));

		addInput(createInput<PJ301MPort>(Vec(knobX1, knobY2), module, LFMModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(knobX3, knobY2), module, LFMModule::RIGHT_INPUT));	

    addInput(createInput<PJ301MPort>(Vec(30, 147), module, LFMModule::NEXT_PRESET_INPUT));

    //std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
    if (module) {
      w = BaseProjectMWidget::create<WindowedProjectMWidget>(Vec(85, 20), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex);
      w->module = module;
      //w->font = font;
      addChild(w);
    }
  }
};

struct EmbeddedLFMModuleWidget : BaseLFMModuleWidget {
    JWModuleResizeHandle *rightHandle;
    BGPanel *panel;
    EmbeddedLFMModuleWidget(LFMModule* module) {

    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Visualizer.svg")));

		panel = new BGPanel(nvgRGB(0, 0, 0));
		panel->box.size = box.size;

		addChild(panel);

    if (module) {
      w = BaseProjectMWidget::create<EmbeddedProjectMWidget>(Vec(85, 0), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex);
      w->module = module;
      w->box.size = Vec(RACK_GRID_HEIGHT,RACK_GRID_HEIGHT);
      addChild(w);

      JWModuleResizeHandle *rightHandle = new JWModuleResizeHandle(w->getRenderer()->window);
		  rightHandle->right = true;
		  this->rightHandle = rightHandle;

		  addChild(rightHandle);
    }

        
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, LFMModule::PARAM_TIMER));
    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, LFMModule::PARAM_NEXT,LFMModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, LFMModule::PARAM_PREV,LFMModule::PREV_LIGHT));
    addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(Vec(buttonX1,buttonY0), module, LFMModule::PARAM_HARD_CUT, LFMModule::HARD_CUT_LIGHT));

		addInput(createInput<PJ301MPort>(Vec(knobX1, knobY2), module, LFMModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(knobX3, knobY2), module, LFMModule::RIGHT_INPUT));	

    addInput(createInput<PJ301MPort>(Vec(30, 147), module, LFMModule::NEXT_PRESET_INPUT));
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

Model *modelWindowedLFMModule = createModel<LFMModule, LFMModuleWidget>("LFMFull");
Model *modelEmbeddedLFMModule = createModel<LFMModule, EmbeddedLFMModuleWidget>("LFMEmbedded");