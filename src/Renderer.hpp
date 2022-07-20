#pragma once
#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "GLFW/glfw3.h"
#ifdef ARCH_WIN
#include "../dep/include/libprojectM/Windows/projectM.h"
#endif
#ifdef ARCH_LIN
#include "../dep/include/libprojectM/Linux/projectM.h"
#endif
#ifdef ARCH_MAC
#include "../dep/include/libprojectM/Mac/projectM.h"
#endif
#include <list>
#include <thread>
#include <mutex>

static const int RENDER_WIDTH = RACK_GRID_WIDTH * 35;
// Special values for preset requests
static const int kPresetIDRandom = -1; // Switch to a random preset
static const int kPresetIDKeep = -2; // Keep the current preset

struct mySettings : projectm_settings {
  int presetIndex;
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
    GLFWwindow* window;
  std::vector<unsigned char> buffer;
  int bufferWidth = RENDER_WIDTH;
  double presetTime = 0;
  bool aspectCorrection = true;
  bool beatSensitivity_up = false;
  bool beatSensitivity_down = false;
  // init creates the OpenGL context to render in, in the main thread,
  // then starts the rendering thread. This can't be done in the ctor
  // because creating the window calls out to virtual methods.
  void init(mySettings const& s);

  // The dtor signals the rendering thread to terminate, then waits
  // for it to do so. It then deletes the OpenGL context in the main
  // thread.
  virtual ~ProjectMRenderer();

  // Sends PCM data to projectM
  void addPCMData(float* data, unsigned int nsamples);

  // Requests that projectM changes the preset at the next opportunity
  void requestPresetID(int id);

  // Requests that projectM changes the autoplay status
  void requestToggleAutoplay();

  // True if projectM is autoplaying presets
  bool isAutoplayEnabled() const;

  // ID of the current preset in projectM's list
  unsigned int activePreset() const;

  // Switches to the previous preset in the current playlist.
  void selectPreviousPreset(bool hard_cut) const;

  // Switches to the next preset in the current playlist.
  void selectNextPreset(bool hard_cut) const;

  // Name of the preset projectM is currently displaying
  std::string activePresetName() const;

  // Returns a list of all presets currently loaded by projectM
  std::list<std::pair<unsigned int, std::string> > listPresets() const;

  // Set the time a preset should last
  void setPresetTime(double);
  
  // Set the aspect correction that is supported by some presets
  void setAspectCorrection(bool);

  // Set the sensitivity 
  void setBeatSensitivity(bool,bool);

  // True if the renderer is currently able to render projectM images
  bool isRendering() const;

protected:
  virtual void extraProjectMInitialization() {}

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
  void renderLoop(mySettings s,std::string);
  void CheckViewportSize(GLFWwindow*);
  virtual GLFWwindow* createWindow() = 0;
  int _renderWidth{ 0 };
  int _renderHeight{ 0 };
};

class WindowedRenderer : public ProjectMRenderer {
public:
  virtual ~WindowedRenderer() {}

private:
  GLFWwindow* createWindow() override;
  int last_xpos, last_ypos, last_width, last_height;
  static void framebufferSizeCallback(GLFWwindow* win, int x, int y);
  static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
};

class TextureRenderer : public ProjectMRenderer {
public:
  virtual ~TextureRenderer() {}
  int getTextureID() const;
  unsigned char* getBuffer();
  int getWindowWidth();
  
private:
  int texture;
  static void framebufferSizeCallback(GLFWwindow* win, int x, int y);
  
  GLFWwindow* createWindow() override;
  void extraProjectMInitialization() override;
};

#endif