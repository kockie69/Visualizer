#define NANOVG_GL2
#include "RPJ.hpp"
#include "nanovg_gl.h"
#include "../dep/include/libprojectM/projectM.h"
#include "Renderer.hpp"
#include "ctrl/RPJKnobs.hpp"
#include "ctrl/RPJButtons.hpp"
#include "JWResizableHandle.hpp"
#include <thread>

static const unsigned int kSampleWindow = 512;

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

struct ImageWidget : TransparentWidget {
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

  LFMModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configButton(PARAM_NEXT, "Next preset");
	  configButton(PARAM_PREV, "Previous preset");
    configParam(PARAM_TIMER, 0.f, 300.f, 30.f, "Time till next preset"," Seconds");
    configParam(PARAM_BEAT_SENS, 0.f, 5.f, 1.f, "Beat sensitivity","");
    configParam(PARAM_HARD_SENS, 0.f, 5.f, 1.f, "Hardcut sensitivity","");
    configParam(PARAM_HARD_DURATION, 0.f, 300.f, 30.f, "Hardcut duration"," Seconds");
    configParam(PARAM_GRADIENT, 0.f, 5.f, 1.f, "Gradient"," ");
  }

  std::vector<std::string> lists = {};
  float presetTime = 0;
  float beatSensitivity = 1;
  float hardcutSensitivity = 1;
  float hardcutDuration = 0;
  float gradient = 1;
  bool aspectCorrection = true;
  int presetIndex = 0;
  int newPresetIndex = 0;
  std::string newPresetName = "";
  std::string activePresetName = "";
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
  bool hardCut = true;
  
  dsp::SchmittTrigger nextInputTrigger,prevInputTrigger;
  dsp::BooleanTrigger nextTrigger,prevTrigger,addTrigger,delTrigger,presetTrigger;
  float pcm_data[kSampleWindow];

  void step() override {
    pcm_data[i++] = inputs[LEFT_INPUT].getVoltage()/5;
    if (inputs[RIGHT_INPUT].isConnected())
      pcm_data[i++] = inputs[RIGHT_INPUT].getVoltage()/5;
    else
      pcm_data[i++] = inputs[LEFT_INPUT].getVoltage()/5;
    if (i >= kSampleWindow) {
      i = 0;
      full = true;
    }
    if (presetTrigger.process(params[PARAM_PRESETTYPE].getValue())) {
      if (!lists.empty()) {
        newPresetName=lists.begin()->data();
        autoPlay=false;
      }
    }
    if (addTrigger.process(params[PARAM_ADDFAV].getValue()) > 0.f && params[PARAM_PRESETTYPE].getValue()==0 )
      addPreset();
    if (delTrigger.process(params[PARAM_DELFAV].getValue()) > 0.f  && params[PARAM_PRESETTYPE].getValue()==1 )
      delPreset();  
    presetTime = params[PARAM_TIMER].getValue();
    beatSensitivity = params[PARAM_BEAT_SENS].getValue();
    if (inputs[BEAT_INPUT].isConnected())
      beatSensitivity+=inputs[BEAT_INPUT].getVoltage();
    hardcutSensitivity = params[PARAM_HARD_SENS].getValue();
    if (inputs[HARDCUT_INPUT].isConnected())
      hardcutSensitivity+=inputs[HARDCUT_INPUT].getVoltage();
    hardcutDuration = params[PARAM_HARD_DURATION].getValue();
    if (inputs[HARDCUT_DURATION_INPUT].isConnected())
      hardcutDuration+=inputs[HARDCUT_DURATION_INPUT].getVoltage();
    gradient = params[PARAM_GRADIENT].getValue();
    if (inputs[GRADIENT_INPUT].isConnected())
      gradient+=inputs[GRADIENT_INPUT].getVoltage();   
    
    if (nextTrigger.process(params[PARAM_NEXT].getValue()) > 0.f || nextInputTrigger.process(rescale(inputs[NEXT_PRESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
      if (params[PARAM_PRESETTYPE].getValue()==0)
        nextPreset=true;
      else {
        if (!lists.empty())
          nextFavourite();
      }
	  }

    if (prevTrigger.process(params[PARAM_PREV].getValue()) > 0.f || prevInputTrigger.process(rescale(inputs[PREV_PRESET_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
      if (params[PARAM_PRESETTYPE].getValue()==0)
        prevPreset=true;
      else {
        if (!lists.empty())
          prevFavourite();
      }
	  }
    
    lights[NEXT_LIGHT].setBrightness(nextPreset);
	  lights[PREV_LIGHT].setBrightness(prevPreset);

  }

  void nextFavourite() {
    auto it = std::find(lists.begin(), lists.end(), activePresetName);
    if (it != lists.end()) { // Found the title
      it++;
      if (it != lists.end()) {
        newPresetName = *it;
      }
      else {
        it = lists.begin();
        newPresetName = *it;
      }
    }
  }

  void prevFavourite() {

    auto it = std::find(lists.begin(), lists.end(), activePresetName);
    if (it != lists.end()) { // Found the title
      if (it != lists.begin()) {
        it--;
        newPresetName=*it;
      }
      else {
        it = lists.end();
        it--;
        newPresetName=*it;
      }
    }
  }

  void addPreset() {
    auto it = std::find(lists.begin(), lists.end(), activePresetName);
    if (it == lists.end())  // Did not find the title
      lists.push_back(activePresetName);
  }

  void delPreset() {
    auto it = std::find(lists.begin(), lists.end(), activePresetName);
    if (it != lists.end()) // Found the title
      lists.erase(it);
    if (!lists.empty())
      newPresetName = lists.begin()->data();
  }

  json_t *dataToJson() override {
	  json_t *rootJ=json_object();
	  json_object_set_new(rootJ, "ActivePreset", json_integer(presetIndex));
    json_object_set_new(rootJ, "DisplayPresetName", json_boolean(displayPresetName));
    json_object_set_new(rootJ, "Autoplay", json_boolean(autoPlay));
    json_object_set_new(rootJ, "CaseSensitiveSearch", json_boolean(caseSensitive));
    json_object_set_new(rootJ, "Aspectcorrection", json_boolean(aspectCorrection)); 
    json_object_set_new(rootJ, "Hardcut", json_boolean(hardCut));
    auto it = lists.begin();
    json_t *listArray=json_array();
    while (it != lists.end()) {
      json_array_append(listArray,json_string(it->data()));
      it++;
    }
    json_object_set_new(rootJ, "List", listArray);
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
    json_t *nListJ = json_object_get(rootJ,"List");
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
    if (nListJ) {
      lists.clear();
      int i=0;
      while (i!=(int)json_array_size(nListJ)) {
        lists.push_back(json_string_value(json_array_get(nListJ,i)));
        i++;
      }
    }
    if (params[LFMModule::PARAM_PRESETTYPE].getValue()!=0) {
      newPresetName=lists.begin()->data();
      autoPlay=false;
    }
  }
};


struct BaseProjectMWidget : FramebufferWidget {

  const int fps = 60;
  const bool debug = true;
  bool oldAutoPlay = false;

  LFMModule* module;

  BaseProjectMWidget() {}

  void init(std::string presetURL,int presetIndex,bool windowed) {
      getRenderer()->init(initSettings(presetURL,presetIndex),&module->windowedXpos,&module->windowedYpos,&module->windowedWidth,&module->windowedHeight,windowed);
  }

  template<typename T>
  static BaseProjectMWidget* create(Vec pos, std::string presetURL,int presetIndex,bool windowed) {
    BaseProjectMWidget* p = new T;
    p->box.pos = pos;
    p->init(presetURL,presetIndex,windowed);
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
      module->activePresetName = getRenderer()->activePresetName().c_str();
      getRenderer()->presetTime = module->presetTime;
      getRenderer()->beatSensitivity = module->beatSensitivity;
      getRenderer()->hardcutSensitivity = module->hardcutSensitivity;
      getRenderer()->hardcutDuration = module->hardcutDuration;
      //getRenderer()->softcutDuration = module->softcutDuration;
      getRenderer()->aspectCorrection = module->aspectCorrection;
      getRenderer()->hardCut = module->hardCut;
      if (module->newPresetName != "") {
        getRenderer()->setRequestPresetName(module->newPresetName);
        module->newPresetName="";
      }
      module->presetIndex = getRenderer()->activePreset();
      if (module->autoPlay != getRenderer()->isAutoplayEnabled())
        getRenderer()->requestToggleAutoplay();
        
      if (module->full) {
        getRenderer()->addPCMData(module->pcm_data, kSampleWindow/2);
        module->full = false;
      }
      //if (getRenderer()->getSwitchPreset()) {
      //  module->nextPreset=true;
      //  getRenderer()->setSwitchPreset(false);
      //}
      if (module->newPresetIndex!=0) {
        getRenderer()->requestPresetID(module->newPresetIndex);
        module->newPresetIndex=0;
      }
      else {
      // If the module requests that we change the preset at random
      // (i.e. the random button was clicked), tell the render thread to
      // do so on the next pass.
        if (module->nextPreset) {
          module->nextPreset = false;
          if (!getRenderer()->isAutoplayEnabled())
            getRenderer()->nextPreset=true;
          else 
            getRenderer()->requestPresetID(kPresetIDRandom);
        }
        if (module->prevPreset) {
          module->prevPreset = false;
          if (!getRenderer()->isAutoplayEnabled())
            getRenderer()->prevPreset=true;
          else
            getRenderer()->requestPresetID(kPresetIDRandom);
        }
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
    
    s.preset_path = (char *)presetURL.c_str();
    s.window_width = RENDER_WIDTH;
    s.window_height = RACK_GRID_HEIGHT;
    s.fps =  60;
    s.mesh_x = 220;
    s.mesh_y = 125;
    s.aspect_correction = true;

    // Preset display settings
    s.preset_duration = 30;
    s.soft_cut_duration = 0;
    s.hard_cut_enabled = true;
    s.hard_cut_duration= 20;
    s.hard_cut_sensitivity =  0.5;
    s.beat_sensitivity = 1;
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
      int x = renderer->getRenderWidth();
      int x2 = renderer->getWindowWidth();
      int b1 = renderer->getBufferSize();
      nvgDeleteImage(args.vg,img);
      if (x == (b1/380/4)) {
        img = nvgCreateImageRGBA(args.vg,x,y,0,renderer->getBuffer());
        std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
    
        NVGpaint imgPaint = nvgImagePattern(args.vg, 0, 0, x, y, 0.0f, img, module->gradient);

        nvgSave(args.vg);
        nvgScale(args.vg, 1, -1); // flip
        nvgTranslate(args.vg,0, -y);
        nvgBeginPath(args.vg);
        // Box is positioned a bit to the left as we otherwise have a small black box on the left
        // nvgRect(args.vg, -10, 0, x+20, y); 
        nvgRect(args.vg, 0, 0, x2, y);
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

template <typename T>
ui::MenuItem* myCreateBoolPtrMenuItem(std::string text, std::string rightText, T* ptr) {
	return createBoolMenuItem(text, rightText,
		[=]() {
			return ptr ? *ptr : false;
		},
		[=](T val) {
			if (ptr)
				*ptr = val;
		}
	);
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

  template <class TMenuItem = ui::MenuItem>
  ui::MenuItem* myCreateBoolMenuItem(std::string text, std::string rightText, std::function<bool()> getter, std::function<void(bool state)> setter, bool disabled = false, bool alwaysConsume = false) {
    struct Item : TMenuItem {
      std::string rightTextPrefix;
      std::function<bool()> getter;
      std::function<void(size_t)> setter;
      bool alwaysConsume;

      void step() override {
        this->rightText = rightTextPrefix;
        if (getter()) {
          if (!rightTextPrefix.empty())
            this->rightText += "  ";
          this->rightText += CHECKMARK_STRING;
        }
        TMenuItem::step();
      }
      void onAction(const event::Action& e) override {
        setter(!getter());
        if (alwaysConsume)
          e.consume(this);
      }
    };

    Item* item = createMenuItem<Item>(text);
    item->rightTextPrefix = rightText;
    item->getter = getter;
    item->setter = setter;
    item->disabled = true;
    item->alwaysConsume = alwaysConsume;
    return item;
  }

  template <typename T>
  ui::MenuItem* myCreateBoolPtrMenuItem(std::string text, std::string rightText, T* ptr) {
    return myCreateBoolMenuItem(text, rightText,
      [=]() {
        return ptr ? *ptr : false;
      },
      [=](T val) {
        if (ptr)
          *ptr = val;
      }
    );
  }


  void appendContextMenu(Menu* menu) override {
    LFMModule* m = dynamic_cast<LFMModule*>(module);
    assert(m);

    // General module settings
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Options"));
    if (!m->params[LFMModule::PARAM_PRESETTYPE].getValue())
      menu->addChild(createBoolPtrMenuItem("Cycle through presets","", &m->autoPlay));
    else
      menu->addChild(myCreateBoolPtrMenuItem("Cycle through presets","", &m->autoPlay));
 
    if (m->getModel()->name == "LFMEmbedded" ) {
      menu->addChild(createBoolPtrMenuItem("Show Preset Title","", &m->displayPresetName));
    }
    menu->addChild(createBoolPtrMenuItem("Hardcut enabled","", &m->hardCut));
    menu->addChild(createBoolPtrMenuItem("Aspectcorrection enabled","", &m->aspectCorrection));
    menu->addChild(createBoolPtrMenuItem("Case sensitive Visual Preset Search","", &m->caseSensitive));

    // Menu items to deal with presets
    menu->addChild(construct<MenuLabel>());
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Visual Presets"));

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

    addParam(createParam<ButtonBig>(Vec(17,40),module, LFMModule::PARAM_PRESETTYPE));
    addParam(createParam<ButtonPlusBig>(Vec(7,55),module, LFMModule::PARAM_ADDFAV));
    addParam(createParam<ButtonMinBig>(Vec(25,55),module, LFMModule::PARAM_DELFAV));     
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, LFMModule::PARAM_TIMER));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY2), module, LFMModule::PARAM_BEAT_SENS));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY3), module, LFMModule::PARAM_HARD_SENS));

    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, LFMModule::PARAM_NEXT,LFMModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, LFMModule::PARAM_PREV,LFMModule::PREV_LIGHT));

   	addInput(createInput<PJ301MPort>(Vec(jackX2, jackY1), module, LFMModule::BEAT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY2), module, LFMModule::HARDCUT_INPUT));	 

    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY5), module, LFMModule::NEXT_PRESET_INPUT));
    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY6), module, LFMModule::PREV_PRESET_INPUT));
    
    addInput(createInput<PJ301MPort>(Vec(jackX1, jackY7), module, LFMModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY7), module, LFMModule::RIGHT_INPUT));

    addParam(createParam<RPJKnob>(Vec(knobX1,knobY4), module, LFMModule::PARAM_HARD_DURATION));	

    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY3), module, LFMModule::HARDCUT_DURATION_INPUT));

    //std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf"));
    if (module) {
      w = BaseProjectMWidget::create<WindowedProjectMWidget>(Vec(85, 20), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex,true);
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
    panel->box.size.x = RACK_GRID_WIDTH;
		addChild(panel);

    if (module) {
      // this is a "live" module in Rack
      w = BaseProjectMWidget::create<EmbeddedProjectMWidget>(Vec(6*RACK_GRID_WIDTH-4, 0), asset::plugin(pluginInstance, "res/presets_projectM/"),module->presetIndex,false);
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


    addParam(createParam<ButtonBig>(Vec(17,40),module, LFMModule::PARAM_PRESETTYPE));
    addParam(createParam<ButtonPlusBig>(Vec(7,55),module, LFMModule::PARAM_ADDFAV));
    addParam(createParam<ButtonMinBig>(Vec(25,55),module, LFMModule::PARAM_DELFAV)); 
    addParam(createParam<RPJKnob>(Vec(knobX2,knobY1), module, LFMModule::PARAM_TIMER));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY2), module, LFMModule::PARAM_BEAT_SENS));
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY3), module, LFMModule::PARAM_HARD_SENS));

    addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY1), module, LFMModule::PARAM_NEXT,LFMModule::NEXT_LIGHT));
		addParam(createLightParamCentered<VCVLightBezel<WhiteLight>>(Vec(buttonX1,buttonY2), module, LFMModule::PARAM_PREV,LFMModule::PREV_LIGHT));

   	addInput(createInput<PJ301MPort>(Vec(jackX2, jackY1), module, LFMModule::BEAT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY2), module, LFMModule::HARDCUT_INPUT));	 

    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY5), module, LFMModule::NEXT_PRESET_INPUT));
    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY6), module, LFMModule::PREV_PRESET_INPUT));
    
    addInput(createInput<PJ301MPort>(Vec(jackX1, jackY7), module, LFMModule::LEFT_INPUT));	
		addInput(createInput<PJ301MPort>(Vec(jackX2, jackY7), module, LFMModule::RIGHT_INPUT));

    addParam(createParam<RPJKnob>(Vec(knobX1,knobY4), module, LFMModule::PARAM_HARD_DURATION));	
    addParam(createParam<RPJKnob>(Vec(knobX1,knobY5), module, LFMModule::PARAM_GRADIENT));

    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY3), module, LFMModule::HARDCUT_DURATION_INPUT));
    addInput(createInput<PJ301MPort>(Vec(jackX2, jackY4), module, LFMModule::GRADIENT_INPUT));
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
