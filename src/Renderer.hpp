#pragma once
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "GLFW/glfw3.h"
#include  "libprojectM/projectM.h"

#include <list>
#include <thread>
#include <mutex>

static const int RENDER_WIDTH = RACK_GRID_WIDTH * 34 + 5;;
// Special values for preset requests
static const int kPresetIDRandom = -1; // Switch to a random preset
static const int kPresetIDKeep = -2; // Keep the current preset

struct mySettings : projectm_settings {
  std::string presetName;
  char* preset_path;
};

class ProjectMRenderer {
public:
  enum Status {
    NOT_INITIALIZED,
    SETTINGS_SET,
    RENDERING,
    FAILED,
    PLEASE_EXIT,
    EXITING
  };

private:

  std::thread renderThread;
  Status status = Status::NOT_INITIALIZED;
  int requestedPresetID = kPresetIDKeep; // Indicates to the render thread that it should switch to the specified preset
  bool requestedToggleAutoplay = false;

  mutable std::mutex pm_m;
  mutable std::mutex flags_m;

  double _sensitivity = 0;

protected:
  projectm* pm = NULL;
  bool dirtySize = false;

public:
  ProjectMRenderer() {}
  std::vector<std::string> fullList = {};
  std::string presetNameActive = "";
  //bool switching = false;
  //bool switchPreset=false;
  std::string newPresetName = "";

  int buttonEvent;
  int cp_x;
  int cp_y;
  int offset_cpx=0;
  int offset_cpy=0;
  int w_posx;
  int w_posy;

  GLsizei bufferSize;
  int windowWidth{ 0 };
  int renderWidth{ 0 };
  GLFWwindow* window;
  std::vector<unsigned char> buffer;
  double presetTime = 0;
  float beatSensitivity = 1;
  float hardcutSensitivity = 1;
  double hardcutDuration = 0;
  double softcutDuration = 0;
  bool firstBeat = true;
  bool aspectCorrection = true;
  bool nextPreset = false;
  bool prevPreset = false;
  bool hardCut = true;
  void setNoFrames(bool);
  void setAlwaysOnTop(bool);
  int *xPos;
  int *yPos;
  int *winWidth;
  int *winHeight;
  static void window_pos_callback(GLFWwindow*, int, int);
  static void window_size_callback(GLFWwindow*, int, int);
  static void mouse_button_callback(GLFWwindow*, int, int, int);
  static void cursor_position_callback(GLFWwindow*, double, double);
  // init creates the OpenGL context to render in, in the main thread,
  // then starts the rendering thread. This can't be done in the ctor
  // because creating the window calls out to virtual methods.
  void init(mySettings const& s,int*,int*,int*,int*,bool,bool,bool);

  // The dtor signals the rendering thread to terminate, then waits
  // for it to do so. It then deletes the OpenGL context in the main
  // thread.
  virtual ~ProjectMRenderer();

  // Sends PCM data to projectM
  void addPCMData(float* data, unsigned int nsamples);

  bool getSwitchPreset();
  void setSwitchPreset(bool);

  void loadPresetItems(std::string);

  static void PresetSwitchedErrorEvent(const char*, const char*, void*);
                                                    
  // Requests that projectM changes the preset at the next opportunity
  void requestPresetID(int id);

  void setRequestPresetName(std::string);

   // Requests that projectM changes the preset at the next opportunity
  void requestPresetName(std::string,bool);

  // Requests that projectM changes the autoplay status
  void requestToggleAutoplay();

  // True if projectM is autoplaying presets
  bool isAutoplayEnabled() const;

  // ID of the current preset in projectM's list
  unsigned int activePreset() const;

  // Switches to the previous preset in the current playlist.
  void selectPreviousPreset(bool hard_cut);

  // Switches to the next preset in the current playlist.
  void selectNextPreset(bool hard_cut);

  // Name of the preset projectM is currently displaying
  std::string activePresetName();

  // Returns a list of all presets currently loaded by projectM
  std::list<std::pair<unsigned int, std::string> > listPresets() const;

  // Set the time a preset should last
  void setPresetTime(double);
  
  // Set the aspect correction that is supported by some presets
  void setAspectCorrection(bool);

  // Set the sensitivity 
  void setBeatSensitivity(float);

  void setHardcutSensitivity(float);

  void setHardcutDuration(double);

  void setSoftcutDuration(double);
  
  // Set the Hardcut
  void setHardcut(bool);

  static void PresetSwitchedEvent(bool, void*);

  // True if the renderer is currently able to render projectM images
  bool isRendering() const;
  virtual void showWindow(int* ,int*,int*,int* ) {};
protected:
  static void logGLFWError(int errcode, const char* errmsg);
  void logContextInfo(std::string name, GLFWwindow* w) const;
private:
  int getClearRequestedPresetID();
  bool getClearRequestedToggleAutoplay();
  Status getStatus() const;
  void setStatus(Status s);
  void renderSetAutoplay(bool enable); // TODO rename this method and other render* methods
  // Switch to the indicated preset. This should be called only from
  // the render thread.
  void renderLoopSetPreset(unsigned int i);
  void renderLoopNextPreset();
  void renderLoop(mySettings s,std::string,bool);
  void CheckViewportSize(GLFWwindow*);
  int renderHeight{ 0 };
  int windowHeight{ 0 };
  virtual GLFWwindow* createWindow(int*,int*,int*,int*,bool,bool) = 0;
};

class WindowedRenderer : public ProjectMRenderer {
public:
  GLFWwindow* c;
  virtual ~WindowedRenderer() {}
  void showWindow(int* ,int*,int*,int* ) override;
private:
  void setPosition(int,int);
  GLFWwindow* createWindow(int*,int*,int*,int*,bool,bool) override;
  int last_xpos, last_ypos, last_width, last_height;
  static void framebufferSizeCallback(GLFWwindow* win, int x, int y);
  static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
};

class TextureRenderer : public ProjectMRenderer {
public:
  virtual ~TextureRenderer() {}
  unsigned char* getBuffer();
  int getWindowWidth();
  int getRenderWidth();
  int getBufferSize();

private:
  GLFWwindow* createWindow(int*,int*,int*,int*,bool,bool) override;
  static void framebufferSizeCallback(GLFWwindow* win, int x, int y);
};

#endif