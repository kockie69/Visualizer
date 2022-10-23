#define NANOVG_GL2
#include "RPJ.hpp"
#include "nanovg_gl.h"
#include "dep/include/libprojectM/projectM.h"
#include "Renderer.hpp"
#include "ctrl/RPJKnobs.hpp"
#include "ctrl/RPJButtons.hpp"
#include "JWResizableHandle.hpp"
#include <thread>

static const unsigned int kSampleWindow = 512;

// Then do the knobs
const float knobX1 = 27;

const float knobY1 = 44;
const float knobY2 = 90;

const float buttonX1 = 41;

const float buttonY0 = 100;
const float buttonY1 = 185;
const float buttonY2 = 215;
const float buttonY3 = 245;
const float buttonY4 = 275;

const float jackX1 = 11;
const float jackX2 = 27;
const float jackX3 = 47;

const float jackY1 = 147;
const float jackY2 = 311;

struct ImageWidget : TransparentWidget
{
  std::string image_file_path;
  float width;
  float height;
  float alpha = 1.0;

  ImageWidget(std::string image_file_path, float width, float height, float alpha = 1.0)
  {
    this->image_file_path = image_file_path;
    this->width = width; // 2154
    this->height = height; // 1525
    this->alpha = alpha;
  }

  void draw(const DrawArgs &args) override
  {
    std::shared_ptr<Image> img = APP->window->loadImage(asset::plugin(pluginInstance, this->image_file_path));

    int temp_width, temp_height;

    // Get the image size and store it in the width and height variables
    nvgImageSize(args.vg, img->handle, &temp_width, &temp_height);

    // Set the bounding box of the widget
    box.size = Vec(width, height);

    // Paint the .png background
    NVGpaint paint = nvgImagePattern(args.vg, 0.0, 0.0, box.size.x, box.size.y, 0.0, img->handle, this->alpha);
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 85.0, 0.0, box.size.x, box.size.y);
    nvgFillPaint(args.vg, paint);
    nvgFill(args.vg);

    Widget::draw(args);
  }
};

struct LFMModule : Module {
  enum ParamIds {
    PARAM_NEXT,
		PARAM_PREV,
    PARAM_TIMER,
    PARAM_BEAT_SENS,
    PARAM_BEAT_SENSE_DOWN,
    PARAM_BEAT_SENSE_UP,
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
    NUM_LIGHTS
  };

  LFMModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configButton(PARAM_NEXT, "Next preset");
	  configButton(PARAM_PREV, "Previous preset");
    configParam(PARAM_TIMER, 0.f, 300.f, 30.f, "Time till next preset"," Seconds");
    configParam(PARAM_BEAT_SENS, 0.f, 5.f, 1.f, "Beat sensitivity","");
    configButton(PARAM_BEAT_SENSE_DOWN, "Decrease beat sensitivity");
    configButton(PARAM_BEAT_SENSE_UP,"Increase beat sensitivity");
  }

  float presetTime = 0;
  bool aspectCorrection = true;
  bool beatSensitivity_up = false;
  bool beatSensitivity_down = false;
  int presetIndex = 0;
  bool displayPresetName = false;
  bool autoPlay = false;
  bool caseSensitive = false;
  unsigned int i = 0;
  bool full = false;
  bool nextPreset = false;
  bool prevPreset = false;
  int windowedXpos = 100;
  int windowedYpos = 100;
  int windowedWidth = 640;
  int windowedHeight = 480;
  // If hardCut is not enabled the rendering will screw up after a while. Not clear yet what is causing this
  bool hardCut = true;
  
  dsp::SchmittTrigger nextInputTrigger;
  dsp::BooleanTrigger nextTrigger,prevTrigger;
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

    beatSensitivity_up = params[PARAM_BEAT_SENSE_UP].getValue();
    beatSensitivity_down = params[PARAM_BEAT_SENSE_DOWN].getValue();

    if (nextTrigger.process(params[PARAM_NEXT].getValue()) > 0.f || nextInputTrigger.process(rescale(inputs[NEXT_PRESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
      nextPreset=true;
	  }

    if (prevTrigger.process(params[PARAM_PREV].getValue()) > 0.f) {
		  prevPreset=true;
	  }
    
    lights[NEXT_LIGHT].setBrightness(nextPreset);
	  lights[PREV_LIGHT].setBrightness(prevPreset);

  }

  json_t *dataToJson() override {
	  json_t *rootJ=json_object();
	  json_object_set_new(rootJ, "ActivePreset", json_integer(presetIndex));
    json_object_set_new(rootJ, "DisplayPresetName", json_boolean(displayPresetName));
    json_object_set_new(rootJ, "Autoplay", json_boolean(autoPlay));
    json_object_set_new(rootJ, "CaseSensitiveSearch", json_boolean(caseSensitive));
    json_object_set_new(rootJ, "Aspectcorrection", json_boolean(aspectCorrection)); 
    json_object_set_new(rootJ, "Hardcut", json_boolean(hardCut));
    if (this->getModel()->getFullName() == "RPJ LFMFull") {
      json_object_set_new(rootJ, "windowedXpos", json_integer(windowedXpos));
      json_object_set_new(rootJ, "windowedYpos", json_integer(windowedYpos));
      json_object_set_new(rootJ, "windowedWidth", json_integer(windowedWidth));
      json_object_set_new(rootJ, "windowedHeight", json_integer(windowedHeight));
    }
	  return rootJ;
  }

  void dataFromJson(json_t *rootJ) override {
	  json_t *nActivePresetJ = json_object_get(rootJ, "ActivePreset");
    json_t *nDisplayPresetNameJ = json_object_get(rootJ, "DisplayPresetName");
    json_t *nAutoplayJ = json_object_get(rootJ, "Autoplay");
    json_t *nCSSJ = json_object_get(rootJ, "CaseSensitiveSearch");
    json_t *nAspectCorrectionJ = json_object_get(rootJ, "Aspectcorrection");
    json_t *nHardcutJ = json_object_get(rootJ, "Hardcut");
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
	  if (nActivePresetJ) {
	    presetIndex = json_integer_value(nActivePresetJ);
    }
    if (nDisplayPresetNameJ) {
	    displayPresetName = json_boolean_value(nDisplayPresetNameJ);
    }
    if (nAutoplayJ) {
	    autoPlay = json_boolean_value(nAutoplayJ);
    }
    if (nCSSJ) {
	    caseSensitive = json_boolean_value(nCSSJ);
    }
    if (nAspectCorrectionJ) {
	    aspectCorrection = json_boolean_value(nAspectCorrectionJ);
    }
    if (nHardcutJ) {
	    hardCut = json_boolean_value(nHardcutJ);
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
      getRenderer()->init(initSettings(presetURL,presetIndex),&module->windowedXpos,&module->windowedYpos,&module->windowedWidth,&module->windowedHeight);
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
      getRenderer()->aspectCorrection = module->aspectCorrection;
      getRenderer()->beatSensitivity_up = module->beatSensitivity_up;
      getRenderer()->beatSensitivity_down = module->beatSensitivity_down;
      getRenderer()->hardCut = module->hardCut;
      module->presetIndex = getRenderer()->activePreset();
      if (module->autoPlay != getRenderer()->isAutoplayEnabled())
        getRenderer()->requestToggleAutoplay();
        
      if (module->full) {
        getRenderer()->addPCMData(module->pcm_data, kSampleWindow/2);
        module->full = false;
      }

     // If the module requests that we change the preset at random
     // (i.e. the random button was clicked), tell the render thread to
     // do so on the next pass.
      if (module->nextPreset) {
        module->nextPreset = false;
        if (!getRenderer()->isAutoplayEnabled())
          //getRenderer()->selectNextPreset(module->hardCut);
          getRenderer()->nextPreset=true;
        else 
          getRenderer()->requestPresetID(kPresetIDRandom);
      }
      if (module->prevPreset) {
        module->prevPreset = false;
        if (!getRenderer()->isAutoplayEnabled())
          //getRenderer()->selectPreviousPreset(module->hardCut);
          getRenderer()->prevPreset=true;
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
    
    s.window_width = RENDER_WIDTH;
    s.window_height = RACK_GRID_HEIGHT;
    s.fps =  60;
    s.mesh_x = 220;
    s.mesh_y = 125;
    s.aspect_correction = true;

    // Preset display settings
    s.preset_duration = 30;
    s.soft_cut_duration = 10;
    s.hard_cut_enabled = true;
    s.hard_cut_duration= 20;
    s.hard_cut_sensitivity =  0.0;
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
  
      NVGpaint imgPaint = nvgImagePattern(args.vg, 0, 0, x, y, 0.0f, img, 1.0f);

      nvgSave(args.vg);
      nvgScale(args.vg, 1, -1); // flip
      nvgTranslate(args.vg,0, -y);
      nvgBeginPath(args.vg);
      // Box is positioned a bit to the left as we otherwise have a small black box on the left
      // nvgRect(args.vg, -10, 0, x+20, y); 
      nvgRect(args.vg, -10, 0, x, y);
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
    if (w->module->caseSensitive) {
      if (text.find(textfield->getText()) != std::string::npos)
        visible=true;
      else
        visible=false;
    }
    else {
      std::string _text;
      std::string _textfield;
      _text = text;
      _textfield = textfield->getText();
      std::transform(_text.begin(), _text.end(), _text.begin(), [](unsigned char c){ return std::tolower(c); });
      std::transform(_textfield.begin(), _textfield.end(), _textfield.begin(), [](unsigned char c){ return std::tolower(c); });
      if (_text.find(_textfield) != std::string::npos)
        visible=true;
      else
        visible=false;
    }
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

  void appendContextMenu(Menu* menu) override {
    LFMModule* m = dynamic_cast<LFMModule*>(module);
    assert(m);

    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Options"));
    menu->addChild(createBoolPtrMenuItem("Cycle through presets","", &m->autoPlay));
    if (m->getModel()->name == "LFMEmbedded" ) {
      //DEBUG("Ok, embedded so we will add option to select a preset title");
      menu->addChild(createBoolPtrMenuItem("Show Preset Title","", &m->displayPresetName));
      //DEBUG("Ok, we have added the option to select a preset title");
    }
    menu->addChild(createBoolPtrMenuItem("Hardcut enabled","", &m->hardCut));
    menu->addChild(createBoolPtrMenuItem("Aspectcorrection enabled","", &m->aspectCorrection));
    menu->addChild(createBoolPtrMenuItem("Case sensitive Visual Preset Search","", &m->caseSensitive));
    menu->addChild(construct<MenuLabel>());
    //DEBUG("Ok, we now add the preset menu title");
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Visual Presets"));
    //DEBUG("Ok, added menu title");
    
    //DEBUG("Ok, we will now get the list of presets");
    auto holder = new rack::Widget;
    holder->box.size.x = 200;
    holder->box.size.y = 20;

    auto textfield = new rack::TextField;
    textfield->box.pos.x = 0;
    textfield->box.size.x = 200;
    textfield->placeholder = "Search presets";
    holder->addChild(textfield);
    menu->addChild(holder);

    menu->addChild(construct<MenuLabel>());
    //auto presets = w->getRenderer()->listPresets();
    //DEBUG("Ok, we have the list of presets");
    auto presets = w->getRenderer()->listPresets();
    for (auto p : presets) 
      menu->addChild(SetPresetMenuItem::construct(p.second, p.first, w,textfield));
  }
};

struct LFMModuleWidget : BaseLFMModuleWidget {

  LFMModuleWidget(LFMModule* module) {
    
    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VisualizerWindow.svg")));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY1), module, LFMModule::PARAM_TIMER));
    //addParam(createParam<RPJKnob>(Vec(knobX1,knobY2), module, LFMModule::PARAM_BEAT_SENS));
    addParam(createParam<ButtonMinBig>(Vec(7,96),module, LFMModule::PARAM_BEAT_SENSE_DOWN));
    addParam(createParam<ButtonPlusBig>(Vec(60,96),module, LFMModule::PARAM_BEAT_SENSE_UP));

    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, LFMModule::PARAM_NEXT,LFMModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, LFMModule::PARAM_PREV,LFMModule::PREV_LIGHT));
    
		addInput(createInput<PJ301MPort>(Vec(jackX1, jackY2), module, LFMModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX3, jackY2), module, LFMModule::RIGHT_INPUT));	

    addInput(createInput<PJ301MPort>(Vec(30, jackY1), module, LFMModule::NEXT_PRESET_INPUT));

    //std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
    if (module) {
      w = BaseProjectMWidget::create<WindowedProjectMWidget>(Vec(85, 20), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex);
      w->module = module;
      w->box.size = Vec(RENDER_WIDTH,RACK_GRID_HEIGHT);
      //w->font = font;
      addChild(w);
      w->getRenderer()->showWindow(&module->windowedXpos,&module->windowedYpos,&module->windowedWidth,&module->windowedHeight);
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
      // this is a "live" module in Rack
      w = BaseProjectMWidget::create<EmbeddedProjectMWidget>(Vec(95, 0), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex);
      w->module = module;
      w->box.size = Vec(RENDER_WIDTH,RACK_GRID_HEIGHT);
      addChild(w);
      JWModuleResizeHandle *rightHandle = new JWModuleResizeHandle(w->getRenderer()->window);
		  rightHandle->right = true;
		  this->rightHandle = rightHandle;

		  addChild(rightHandle);
    }
    else {
      // this is the preview in Rack's module browser
      	  std::string imagePath = "res/LFMBackground-3.png";

		      //ImageWidget *display = new ImageWidget(imagePath,RACK_GRID_WIDTH*MODULE_WIDTH,RACK_GRID_HEIGHT);
		      ImageWidget *display = new ImageWidget(imagePath,800,RACK_GRID_HEIGHT);

          addChild(display);
    }


        
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY1), module, LFMModule::PARAM_TIMER));
    //addParam(createParam<RPJKnob>(Vec(knobX1,knobY2), module, LFMModule::PARAM_BEAT_SENS));

    addParam(createParam<ButtonMinBig>(Vec(7,96),module, LFMModule::PARAM_BEAT_SENSE_DOWN));
    addParam(createParam<ButtonPlusBig>(Vec(60,96),module, LFMModule::PARAM_BEAT_SENSE_UP));

    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, LFMModule::PARAM_NEXT,LFMModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, LFMModule::PARAM_PREV,LFMModule::PREV_LIGHT));
    
		addInput(createInput<PJ301MPort>(Vec(jackX1, jackY2), module, LFMModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX3, jackY2), module, LFMModule::RIGHT_INPUT));	

    addInput(createInput<PJ301MPort>(Vec(30, jackY1), module, LFMModule::NEXT_PRESET_INPUT));
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
