#include "rack.hpp"
#define NANOVG_GL2
#include "RPJ.hpp"
#include "nanovg_gl.h"
#include "ctrl/RPJKnobs.hpp"
#include "ctrl/RPJButtons.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"


static const int BUFFER_SIZE = 256;

const unsigned int SCR_WIDTH = 640;
const unsigned int SCR_HEIGHT = 480;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;

bool alwaysOnTop = true;
bool noFrames = false;
int windowedXpos = 100;
int windowedYpos = 100;
float windowedWidth = SCR_WIDTH;
float windowedHeight = SCR_HEIGHT;

struct ScopeModule : Module
{
  enum ParamIds
  {
		X_SCALE_PARAM,
		X_POS_PARAM,
		Y_SCALE_PARAM,
		Y_POS_PARAM,
		TIME_PARAM,
		LISSAJOUS_PARAM,
		THRESH_PARAM,
		TRIG_PARAM,
		NUM_PARAMS
  };
  enum InputIds
  {
		X_INPUT,
		Y_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
  };
  enum OutputIds
  {
		X_OUTPUT,
		Y_OUTPUT,
		NUM_OUTPUTS
  };
  enum LightIds
  {
		LISSAJOUS_LIGHT,
		TRIG_LIGHT,
		NUM_LIGHTS
  };

  	struct Point {
		float min = INFINITY;
		float max = -INFINITY;
	};

	Point pointBuffer[BUFFER_SIZE][2][PORT_MAX_CHANNELS];
	Point currentPoint[2][PORT_MAX_CHANNELS];
	int channelsX = 0;
	int channelsY = 0;
	int bufferIndex = 0;
	int frameIndex = 0;

  dsp::SchmittTrigger triggers[16];

  ScopeModule()
  {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(X_SCALE_PARAM, 0.f, 8.f, 0.f, "Gain 1", " V/screen", 1 / 2.f, 20);
		getParamQuantity(X_SCALE_PARAM)->snapEnabled = true;
		configParam(X_POS_PARAM, -10.f, 10.f, 0.f, "Offset 1", " V");
		configParam(Y_SCALE_PARAM, 0.f, 8.f, 0.f, "Gain 2", " V/screen", 1 / 2.f, 20);
		getParamQuantity(Y_SCALE_PARAM)->snapEnabled = true;
		configParam(Y_POS_PARAM, -10.f, 10.f, 0.f, "Offset 2", " V");
		const float maxTime = -std::log2(5e1f);
		const float minTime = -std::log2(5e-3f);
		const float defaultTime = -std::log2(5e-1f);
		configParam(TIME_PARAM, maxTime, minTime, defaultTime, "Time", " ms/screen", 1 / 2.f, 1000);
		configSwitch(LISSAJOUS_PARAM, 0.f, 1.f, 0.f, "Scope mode", {"1 & 2", "1 x 2"});
		configParam(THRESH_PARAM, -10.f, 10.f, 0.f, "Trigger threshold", " V");
		configSwitch(TRIG_PARAM, 0.f, 1.f, 1.f, "Trigger", {"Enabled", "Disabled"});

		configInput(X_INPUT, "Ch 1");
		configInput(Y_INPUT, "Ch 2");
		configInput(TRIG_INPUT, "External trigger");

		configOutput(X_OUTPUT, "Ch 1");
		configOutput(Y_OUTPUT, "Ch 2");
  }

	void onReset() override {
		for (int i = 0; i < BUFFER_SIZE; i++) {
			for (int w = 0; w < 2; w++) {
				for (int c = 0; c < 16; c++) {
					pointBuffer[i][w][c] = Point();
				}
			}
		}
	}

  void process(const ProcessArgs& args) override {
		bool lissajous = params[LISSAJOUS_PARAM].getValue();
		lights[LISSAJOUS_LIGHT].setBrightness(lissajous);

		bool trig = !params[TRIG_PARAM].getValue();
		lights[TRIG_LIGHT].setBrightness(trig);

		// Detect trigger if no longer recording
		if (bufferIndex >= BUFFER_SIZE) {
			bool triggered = false;

			// Trigger immediately in Lissajous mode, or if trigger detection is disabled
			if (lissajous || !trig) {
				triggered = true;
			}
			else {
				// Reset if triggered
				float trigThreshold = params[THRESH_PARAM].getValue();
				Input& trigInput = inputs[TRIG_INPUT].isConnected() ? inputs[TRIG_INPUT] : inputs[X_INPUT];

				// This may be 0
				int trigChannels = trigInput.getChannels();
				for (int c = 0; c < trigChannels; c++) {
					float trigVoltage = trigInput.getVoltage(c);
					if (triggers[c].process(rescale(trigVoltage, trigThreshold, trigThreshold + 0.001f, 0.f, 1.f))) {
						triggered = true;
					}
				}
			}

			if (triggered) {
				for (int c = 0; c < 16; c++) {
					triggers[c].reset();
				}
				bufferIndex = 0;
				frameIndex = 0;
			}
		}

		// Set channels
		int channelsX = inputs[X_INPUT].getChannels();
		if (channelsX != this->channelsX) {
			// TODO
			// std::memset(bufferX, 0, sizeof(bufferX));
			this->channelsX = channelsX;
		}

		int channelsY = inputs[Y_INPUT].getChannels();
		if (channelsY != this->channelsY) {
			// TODO
			// std::memset(bufferY, 0, sizeof(bufferY));
			this->channelsY = channelsY;
		}

		// Copy inputs to outputs
		outputs[X_OUTPUT].setChannels(channelsX);
		outputs[X_OUTPUT].writeVoltages(inputs[X_INPUT].getVoltages());
		outputs[Y_OUTPUT].setChannels(channelsY);
		outputs[Y_OUTPUT].writeVoltages(inputs[Y_INPUT].getVoltages());

		// Add point to buffer if recording
		if (bufferIndex < BUFFER_SIZE) {
			// Compute time
			float deltaTime = dsp::exp2_taylor5(-params[TIME_PARAM].getValue()) / BUFFER_SIZE;
			int frameCount = (int) std::ceil(deltaTime * args.sampleRate);

			// Get input
			for (int c = 0; c < channelsX; c++) {
				float x = inputs[X_INPUT].getVoltage(c);
				currentPoint[0][c].min = std::min(currentPoint[0][c].min, x);
				currentPoint[0][c].max = std::max(currentPoint[0][c].max, x);
			}
			for (int c = 0; c < channelsY; c++) {
				float y = inputs[Y_INPUT].getVoltage(c);
				currentPoint[1][c].min = std::min(currentPoint[1][c].min, y);
				currentPoint[1][c].max = std::max(currentPoint[1][c].max, y);
			}

			if (++frameIndex >= frameCount) {
				frameIndex = 0;
				// Push current point
				for (int w = 0; w < 2; w++) {
					for (int c = 0; c < 16; c++) {
						pointBuffer[bufferIndex][w][c] = currentPoint[w][c];
					}
				}
				// Reset current point
				for (int w = 0; w < 2; w++) {
					for (int c = 0; c < 16; c++) {
						currentPoint[w][c] = Point();
					}
				}
				bufferIndex++;
			}
		}
	}

	bool isLissajous() {
		return params[LISSAJOUS_PARAM].getValue() > 0.f;
	}

	void dataFromJson(json_t* rootJ) override {
		// In <2.0, lissajous and external were class variables
		json_t* lissajousJ = json_object_get(rootJ, "lissajous");
		if (lissajousJ) {
			if (json_integer_value(lissajousJ))
				params[LISSAJOUS_PARAM].setValue(1.f);
		}

		json_t* externalJ = json_object_get(rootJ, "external");
		if (externalJ) {
			if (json_integer_value(externalJ))
				params[TRIG_PARAM].setValue(1.f);
		}
	}
};

ScopeModule::Point DEMO_POINT_BUFFER[BUFFER_SIZE];

void demoPointBufferInit() {
	static bool init = false;
	if (init)
		return;
	init = true;

	// Calculate demo point buffer
	for (size_t i = 0; i < BUFFER_SIZE; i++) {
		float phase = float(i) / BUFFER_SIZE;
		ScopeModule::Point point;
		point.min = point.max = 4.f * std::sin(2 * M_PI * phase * 2.f);
		DEMO_POINT_BUFFER[i] = point;
	}
}

struct Display : OpenGlWidget
{
  GLuint program, shaderProgram;
  GLint attribute_coord;
  GLint uniform_tex;
  GLint uniform_color;

  struct point
  {
    GLfloat x;
    GLfloat y;
    GLfloat s;
    GLfloat t;
  };

  GLuint vbo;

  const char *fontfilename;
  unsigned int VAO;

  GLFWwindow *c;

  ScopeModule *module;
  ModuleWidget* moduleWidget;
  Shader lineShader = Shader("","");
  Shader textShader = Shader("","");

	int statsFrame = 0;

	struct Stats {
		float min = INFINITY;
		float max = -INFINITY;
	};
	Stats statsX;
	Stats statsY;

  Display(Vec size, bool alwaysOnTop, bool noFrames)
  { 
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    c = glfwCreateWindow(windowedWidth, windowedHeight, "External Scope", NULL, NULL);
    if (!c)
      return;
    glfwMakeContextCurrent(c);
    lineShader = Shader(asset::plugin(pluginInstance, "res/line.vs").c_str(), asset::plugin(pluginInstance, "res/line.fs").c_str());
    textShader = Shader(asset::plugin(pluginInstance, "res/text.vs").c_str(), asset::plugin(pluginInstance, "res/text.fs").c_str());
    initText(textShader);
    glfwSetWindowUserPointer(c, this);
    glfwSetWindowCloseCallback(c, [](GLFWwindow *w) { glfwIconifyWindow(w); });
    
    int width, height;

    glfwGetFramebufferSize(c, &width, &height);
    glViewport(0.0, 0.0, width, height);
    glfwSetWindowAttrib(c,GLFW_FLOATING,alwaysOnTop);
  }

  ~Display() {
    glfwDestroyWindow(c);
    glFinish();
  }

  void drawBackground(Shader shader) {

       // First Line
    float vertices[] = {
        -1.0f, 0.95f, 0.0f,
        1.0f, 0.95f, 0.0f,
        -1.0f, 0.8f, 0.0f,
        1.0f, 0.8f, 0.0f,
        -1.0f, 0.4f, 0.0f,
        1.0f, 0.4f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        -1.0f, -0.4, 0.0f,
        1.0f, -0.4f, 0.0f,
        -1.0f, -0.8, 0.0f,
        1.0f, -0.8f, 0.0f,
        -1.0f, -0.95, 0.0f,
        1.0f, -0.95f, 0.0f};

    float colors[4] = {0.25f,0.25f,0.25f,1.0f};
    unsigned int VBO,VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); 
  
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    shader.use();
    int uniformLocation = glGetUniformLocation(lineShader.ID, "uniformColor");
    if(uniformLocation < 0) {
        // Uniform locations must be 0 or greater, otherwise the uniform was not found in the shader or some other error occured
        // TODO: handle exception
    }
    else {
        glUniform4f(uniformLocation, colors[0], colors[1], colors[2], colors[3]);
    }
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_LINES, 0, 14);  
  }

	void drawStats(Vec pos, const char* title, const Stats& stats) {

		//pos = pos.plus(Vec(20, 11));

    RenderText(textShader, title, pos.x, pos.y, 0.5f, glm::vec3(0.375f, 0.375f, 0.375f));

		std::string text;
		text = "pp ";
		float pp = stats.max - stats.min;
		text += isNear(pp, 0.f, 100.f) ? string::f("% 6.2f", pp) : "  ---";
    RenderText(textShader,text,pos.x+30, pos.y, 0.5f, glm::vec3(0.625f, 0.625f, 0.625f));
		text = "max";
		text += isNear(stats.max, 0.f, 100.f) ? string::f("% 6.2f", stats.max) : "  ---";

    RenderText(textShader,text,pos.x + 240, pos.y, 0.5f, glm::vec3(0.625f, 0.625f, 0.625f));
		text = "min";
		text += isNear(stats.min, 0.f, 100.f) ? string::f("% 6.2f", stats.min) : "  ---";

    RenderText(textShader,text,pos.x + 480, pos.y, 0.5f, glm::vec3(0.625f, 0.625f, 0.625f));
	}

void drawArray(float vertices[BUFFER_SIZE*4],int size,float colors[4],bool strip) {
    float line_width = 1.5;

    glLineWidth((GLfloat)line_width);
    unsigned int VBO,VAO;
    glGenVertexArrays(1, &VAO);
    
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO); 
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size*sizeof(float), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
     glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
     glBindVertexArray(0);

    lineShader.use();
    int uniformLocation = glGetUniformLocation(lineShader.ID, "uniformColor");
    if(uniformLocation < 0) {
        // Uniform locations must be 0 or greater, otherwise the uniform was not found in the shader or some other error occured
        // TODO: handle exception
    }
    else {
        glUniform4f(uniformLocation, colors[0], colors[1], colors[2], colors[3]);
    }
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    if (strip)
      glDrawArrays(GL_LINE_STRIP, 0, size/2);
    else
      glDrawArrays(GL_LINE, 0, size/2);
    glDisableVertexAttribArray(0);
}

void drawWave(int wave, int channel, float offset, float gain, NVGcolor color) {
  ScopeModule::Point pointBuffer[BUFFER_SIZE];
  for (int i = 0; i < BUFFER_SIZE-1; i++) {
    pointBuffer[i] = module ? module->pointBuffer[i][wave][channel] : DEMO_POINT_BUFFER[i];
  }
  float colors[4] = {color.r,color.g,color.b,color.a};

  // Draw max points on top
  int j=0;
  float vertices[BUFFER_SIZE*4];

  for (int i = 0; i < BUFFER_SIZE; i++) {
    const ScopeModule::Point& point = pointBuffer[i];
    float max = point.max;
    if (!std::isfinite(max))
      max = 0.f;
    float min = point.min;
    if (!std::isfinite(min))
      min = 0.f;
    Vec p;
    p.x = (float) 2.0f * i / (BUFFER_SIZE-1);
    p.y = (max + offset) * gain * 0.8f;

    vertices[j] = p.x - 1.0f;
    vertices[j+1] = p.y;

    j = j + 2;

    p.y = (min + offset) * gain * 0.8f;

    vertices[j] = p.x - 1.0f;
    vertices[j+1] = p.y;

    j = j + 2;
  }

  //drawArray(vertices,(BUFFER_SIZE*4)-1,colors,true);
drawArray(vertices,BUFFER_SIZE*4,colors,true);
  }

// render line of text
// -------------------
void RenderText(Shader shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void initText(Shader shader) {
    
    // compile and setup the shader
    // ----------------------------
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    textShader.use();
    glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

	// find path to font
    std::string font_name = asset::plugin(pluginInstance, "res/fonts/LiberationSans/LiberationSans-Regular.ttf");
  //  std::string font_name = FileSystem::getPath("resources/fonts/Antonio-Bold.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return;
    }
	
	// load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 60);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char cha = 0; cha < 128; cha++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, cha, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(cha, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

void calculateStats(Stats& stats, int wave, int channels) {
		if (!module) {
			stats.min = -5.f;
			stats.max = 5.f;
			return;
		}

		stats = Stats();
		for (int i = 0; i < BUFFER_SIZE; i++) {
			for (int c = 0; c < channels; c++) {
				ScopeModule::Point point = module->pointBuffer[i][wave][c];
				stats.max = std::fmax(stats.max, point.max);
				stats.min = std::fmin(stats.min, point.min);
			}
		}
	}

void drawLissajous(int channel, float offsetX, float gainX, float offsetY, float gainY) {
		if (!module)
			return;
    float vertices[BUFFER_SIZE*4];
    float colors[4]={1.0f, 0.84f, 0.08f, 1.0f};

		ScopeModule::Point pointBufferX[BUFFER_SIZE];
		ScopeModule::Point pointBufferY[BUFFER_SIZE];
		for (int i = 0; i < BUFFER_SIZE; i++) {
			pointBufferX[i] = module->pointBuffer[i][0][channel];
			pointBufferY[i] = module->pointBuffer[i][1][channel];
		}
    int j=0;
		int bufferIndex = module->bufferIndex;
		for (int i = 0; i < BUFFER_SIZE; i++) {
			// Get average point
			const ScopeModule::Point& pointX = pointBufferX[(i + bufferIndex) % BUFFER_SIZE];
			const ScopeModule::Point& pointY = pointBufferY[(i + bufferIndex) % BUFFER_SIZE];
			float avgX = (pointX.min + pointX.max) / 2;
			float avgY = (pointY.min + pointY.max) / 2;
			if (!std::isfinite(avgX) || !std::isfinite(avgY))
				continue;

			Vec p;
			p.x = (avgX + offsetX) * gainX * 0.8f;
			p.y = (avgY + offsetY) * gainY * 0.8f;
      vertices[j]=p.y;
      vertices[j+1]=p.x;
      j = j + 2;
		}
    drawArray(vertices,BUFFER_SIZE,colors,true);
	}

  void drawTrig(float value) {

	}

  void drawFramebuffer() override
  {
    int width, height;
    GLFWwindow* win = glfwGetCurrentContext();
    
    // render
    // ------
    glfwMakeContextCurrent(c);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwGetFramebufferSize(c,&width,&height);
    glViewport(0.0, 0.0, width, height);

    drawBackground(lineShader);

		// Calculate and draw stats
    int channelsY = module ? module->channelsY : 1;
		int channelsX = module ? module->channelsX : 1;
		if (statsFrame == 0) {
			calculateStats(statsX, 0, channelsX);
			calculateStats(statsY, 1, channelsY);
		}
		statsFrame = (statsFrame + 1) % 4;

		drawStats(Vec(10.0f, 435.0f), "1", statsX);
		drawStats(Vec(10.0f, 15.0f), "2", statsY);

    glViewport(0.0, height*0.1f, width, height-(height*0.2f));

    float gainX = module ? module->params[ScopeModule::X_SCALE_PARAM].getValue() : 0.f;
		gainX = std::pow(2.f, std::round(gainX)) / 10.f;
		float gainY = module ? module->params[ScopeModule::Y_SCALE_PARAM].getValue() : 0.f;
		gainY = std::pow(2.f, std::round(gainY)) / 10.f;
		float offsetX = module ? module->params[ScopeModule::X_POS_PARAM].getValue() : 5.f;
		float offsetY = module ? module->params[ScopeModule::Y_POS_PARAM].getValue() : -5.f;

		// Get input colors
		PortWidget* inputX = moduleWidget->getInput(ScopeModule::X_INPUT);
		PortWidget* inputY = moduleWidget->getInput(ScopeModule::Y_INPUT);
		CableWidget* inputXCable = APP->scene->rack->getTopCable(inputX);
		CableWidget* inputYCable = APP->scene->rack->getTopCable(inputY);
		NVGcolor inputXColor = inputXCable ? inputXCable->color : SCHEME_YELLOW;
		NVGcolor inputYColor = inputYCable ? inputYCable->color : SCHEME_YELLOW;

		// Draw waveforms

		if (module && module->isLissajous()) {
			// X x Y
			int lissajousChannels = std::min(channelsX, channelsY);
			for (int c = 0; c < lissajousChannels; c++) {
				drawLissajous(c,offsetX, gainX, offsetY, gainY);
			}
		}
		else {
			// Y
			for (int c = 0; c < channelsY; c++) {
				drawWave(1, c, offsetY, gainY,inputYColor);
			}

			// X
			for (int c = 0; c < channelsX; c++) {
				drawWave(0, c, offsetX, gainX,inputXColor);
			}

			// Trigger
			float trigThreshold = module ? module->params[ScopeModule::THRESH_PARAM].getValue() : 0.f;
			trigThreshold = (trigThreshold + offsetX) * gainX;
			//drawTrig(trigThreshold);
		}

    glfwSwapBuffers(c);
    glfwMakeContextCurrent(win);
    FramebufferWidget::drawFramebuffer();
  }

  void free_resources()
  {
    glDeleteProgram(program);
  }

  void onRemove(const RemoveEvent &e) override
  {
    OpenGlWidget::onRemove(e);
  }

  void step() override
  {
    dirty = true;
    FramebufferWidget::step();
  }
};

struct ScopeModuleWidget : ModuleWidget
{
  GLFWwindow *c;
  Display *display;

  ScopeModuleWidget(ScopeModule *module)
  {

    setModule(module);
    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ExternalScope.svg")));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(24.603, 12.819)), module, ScopeModule::LISSAJOUS_PARAM, ScopeModule::LISSAJOUS_LIGHT));
		addParam(createParamCentered<RPJKnob>(mm2px(Vec(8.643, 30.551)), module, ScopeModule::X_SCALE_PARAM));
		addParam(createParamCentered<RPJKnob>(mm2px(Vec(31.023, 30.551)), module, ScopeModule::Y_SCALE_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(35.521, 12.819)), module, ScopeModule::TRIG_PARAM, ScopeModule::TRIG_LIGHT));
		addParam(createParamCentered<RPJKnob>(mm2px(Vec(8.643, 13.819)), module, ScopeModule::TIME_PARAM));
		addParam(createParamCentered<RPJKnob>(mm2px(Vec(8.643, 45.789)), module, ScopeModule::X_POS_PARAM));
		addParam(createParamCentered<RPJKnob>(mm2px(Vec(31.023, 45.789)), module, ScopeModule::Y_POS_PARAM));
		addParam(createParamCentered<RPJKnob>(mm2px(Vec(8.643, 68.815)), module, ScopeModule::THRESH_PARAM));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(8.643, 115.115)), module, ScopeModule::X_INPUT));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(31.023, 115.115)), module, ScopeModule::Y_INPUT));
		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(31.023, 68.815)), module, ScopeModule::TRIG_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(8.643, 95.115)), module, ScopeModule::X_OUTPUT));
		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(31.023, 95.115)), module, ScopeModule::Y_OUTPUT));

    if (module)
    {
      display = new Display(Vec(windowedWidth, windowedHeight), alwaysOnTop, noFrames);
      display->module = module;
      display->moduleWidget = this;
      addChild(display);
    }
  }

  void step() override
  {
    if (module)
    {
      display->drawFramebuffer();
    }
    ModuleWidget::step();
  }
};

Model *modelScopeModule = createModel<ScopeModule, ScopeModuleWidget>("ExternalScope");