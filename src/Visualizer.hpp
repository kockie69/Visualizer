#include "rack.hpp"
#include "../dep/include/libprojectM/projectM.h"

using namespace rack;

const int MODULE_WIDTH=31;
const int WIDTH=370;
const int HEIGHT=370;
const int PRESETDISPLAYLENGTH=12;


struct myRenderer {
	myRenderer();
	~myRenderer();
	std::vector<std::string> getPresets();
	void handleWindowSize();
	void handlePreset();
	std::string getNamePreset();
	void draw();
	projectm_handle projectMHandle;
	projectm_settings *settings;
	float prevZoomlevel;
	size_t renderWidth;
	size_t renderHeight;
	int index,currentIndex;
	float pcmData[2];
	bool locked;
	bool hard_cut;
};

struct RPJVisualizer : Module {

	enum ParamIds {
		PARAM_NEXT,
		PARAM_PREV,
		PARAM_LOCK,
		PARAM_HARD_CUT,
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
		myRenderer renderer;
		bool hard_cut_old;
		bool prev,next;
		
		bool locked;
		dsp::ClockDivider lightDivider;
		int rating_list[3] = {1,2,3};
		unsigned int currentIndex;
};

struct Display : OpenGlWidget {
	Display();
	~Display();
	void drawFramebuffer() override;
	void step() override;

	RPJVisualizer *module;
	projectm *pm;
};

struct PresetNameDisplay : TransparentWidget {
	std::shared_ptr<Font> font;
	NVGcolor txtCol;
	RPJVisualizer* module;
	const int fh = 12; // font height
	dsp::ClockDivider textDivider;
	int txtStart;

	PresetNameDisplay(Vec pos) {
		txtStart=0;
		textDivider.setDivision(20);
		box.pos = pos;
		box.size.y = fh;
		box.size.x = 18;
		setColor(0xff, 0xff, 0xff, 0xff);
		//font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DejaVuSansMono.ttf"));
	}

	PresetNameDisplay(Vec pos, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		txtStart=0;
		textDivider.setDivision(20);
		box.pos = pos;
		box.size.y = fh;
		box.size.x = 18;
		setColor(r, g, b, a);
		//font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DejaVuSansMono.ttf"));
	}

	void setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
		txtCol.r = r;
		txtCol.g = g;
		txtCol.b = b;
		txtCol.a = a;
	}

	void drawLayer(const DrawArgs &args, int layer) override {
		if (layer == 1) {
			char tbuf[15];
			if (module == NULL)
				std::snprintf(tbuf, sizeof(tbuf), "%s", "Marbles");
			else {
				std::string txt;
				if (module->renderer.getPresets().size() >0) {
					txt = module->renderer.getNamePreset();
					if (textDivider.process()) {
						txtStart++;
						if ((txtStart+PRESETDISPLAYLENGTH)>(int)txt.length())
							txtStart=0;
					}
				}

				txt = txt.substr(txtStart,PRESETDISPLAYLENGTH);
				std::snprintf(tbuf, sizeof(tbuf), "%s", txt.c_str());
			}
			TransparentWidget::draw(args);
			drawBackground(args);
			drawValue(args, tbuf);
		}
		TransparentWidget::drawLayer(args,layer);
	}

	void drawBackground(const DrawArgs &args) {
		Vec c = Vec(box.size.x/2, box.size.y);
		int whalf = 1.25*box.size.x;
		int hfh = floor(fh / 2);

		// Draw rounded rectangle
		nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0xff));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, c.x -whalf, c.y +2);
			nvgLineTo(args.vg, c.x +whalf, c.y +2);
			nvgQuadTo(args.vg, c.x +whalf +5, c.y +2+hfh, c.x +whalf, c.y+fh+2);
			nvgLineTo(args.vg, c.x -whalf, c.y+fh+2);
			nvgQuadTo(args.vg, c.x -whalf -5, c.y +2+hfh, c.x -whalf, c.y +2);
			nvgClosePath(args.vg);
		}
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, 0x0F));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
	}

	void drawValue(const DrawArgs &args, const char * txt) {
		Vec c = Vec(box.size.x/2, box.size.y);
		std::shared_ptr<Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DejaVuSansMono.ttf"));

		nvgFontSize(args.vg, fh);
		if (font)
			nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, -2);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGBA(txtCol.r, txtCol.g, txtCol.b, txtCol.a));
		nvgText(args.vg, c.x, c.y+fh-1, txt, NULL);
	}
};
