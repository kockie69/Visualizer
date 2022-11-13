#define NANOVG_GL2
#include "RPJ.hpp"
#include "Renderer.hpp"
#include "GLFW/glfw3.h"
#include "../dep/include/libprojectM/projectM.h"
#include "GlfwUtils.hpp"
#include <thread>
#include <mutex>

void ProjectMRenderer::init(mySettings const& s,int *xpos, int *ypos,int *width,int *height,bool windowed) {
  window = createWindow(xpos,ypos,width,height);
  std::string url = s.preset_path;
  renderThread = std::thread([this,s,url,windowed](){ this->renderLoop(s,url,windowed); });
}

ProjectMRenderer::~ProjectMRenderer() {
  // Request that the render thread terminates the renderLoop
  setStatus(Status::PLEASE_EXIT);
  // Wait for renderLoop to terminate before releasing resources
  renderThread.join();
  // Destroy the window in the main thread, because it's not legal
  // to do so in other threads.
  glfwDestroyWindow(window);
}

void ProjectMRenderer::addPCMData(float* data, unsigned int nsamples) {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_pcm_add_float(pm,data,nsamples,PROJECTM_STEREO);
}

// Requests that projectM changes the preset at the next opportunity
void ProjectMRenderer::requestPresetID(int id) {
  std::lock_guard<std::mutex> l(flags_m);
  requestedPresetID = id;
}

// Requests that projectM changes the autoplay status
void ProjectMRenderer::requestToggleAutoplay() {
  std::lock_guard<std::mutex> l(flags_m);
  requestedToggleAutoplay = true;
}

// True if projectM is autoplaying presets
bool ProjectMRenderer::isAutoplayEnabled() const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return false;
  return !(projectm_is_preset_locked(pm));
}

// Switches to the previous preset in the current playlist.
void ProjectMRenderer::selectPreviousPreset(bool hard_cut) const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_select_previous_preset(pm,hard_cut);
  //projectm_lock_preset(pm,true); 
}

// Switches to the next preset in the current playlist.
void ProjectMRenderer::selectNextPreset(bool hard_cut) const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_select_next_preset(pm,hard_cut);
  //projectm_lock_preset(pm,true);
}

// ID of the current preset in projectM's list
unsigned int ProjectMRenderer::activePreset() const {
  unsigned int presetIdx;
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return 0;
  projectm_get_selected_preset_index(pm,&presetIdx);
  return presetIdx;
}

// Name of the preset projectM is currently displaying
std::string ProjectMRenderer::activePresetName() const {
  unsigned int presetIdx;
  std::unique_lock<std::mutex> l(pm_m);
  if (!pm) return "";
  if (projectm_get_selected_preset_index(pm,&presetIdx)) {
    l.unlock();
    return projectm_get_preset_name(pm,presetIdx);
  }
  return "";
}

void ProjectMRenderer::setPresetTime(double time) {
  std::unique_lock<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_set_preset_duration(pm,time);
}

void ProjectMRenderer::setAspectCorrection(bool correction) {
    std::unique_lock<std::mutex> l(pm_m);
    if (!pm) return;
    projectm_set_aspect_correction(pm, correction);
}

void ProjectMRenderer::setBeatSensitivity(float s) {
  if (!pm) return;
  std::lock_guard<std::mutex> l(pm_m);
  projectm_set_beat_sensitivity(pm,s);
}

void ProjectMRenderer::setHardcutSensitivity(float s) {
  if (!pm) return;
  std::lock_guard<std::mutex> l(pm_m);
  projectm_set_hard_cut_sensitivity(pm,s);
}

void ProjectMRenderer::setHardcutDuration(double d) {
  if (!pm) return;
  std::lock_guard<std::mutex> l(pm_m);
  projectm_set_hard_cut_duration(pm,d);
}

void ProjectMRenderer::setSoftcutDuration(double d) {
  if (!pm) return;
  std::lock_guard<std::mutex> l(pm_m);
  projectm_set_soft_cut_duration(pm,d);
}

void ProjectMRenderer::setHardcut(bool hardCut) {
  if (!pm) return;
  std::lock_guard<std::mutex> l(pm_m);
  bool _hardCut = projectm_get_hard_cut_enabled(pm);
  if (hardCut!=_hardCut)
    projectm_set_hard_cut_enabled(pm,hardCut);
}

// Returns a list of all presets currently loaded by projectM
std::list<std::pair<unsigned int, std::string> > ProjectMRenderer::listPresets() const {
  std::list<std::pair<unsigned int, std::string> > presets;
  
  //DEBUG("Ok, lets check the number of presets found");
  unsigned int n;
  {
    std::lock_guard<std::mutex> l(pm_m);
    if (!pm) return presets;
      n = projectm_get_playlist_size(pm);
  }
  //DEBUG("The number of presets found is %d", n);
  if (!n) {
    return presets;
  }
  for (unsigned int i = 0; i < n; ++i){
    std::string s;
    {
      std::lock_guard<std::mutex> l(pm_m);
      s = projectm_get_preset_name(pm,i);
    }
    presets.push_back(std::make_pair(i, std::string(s)));
  }
  return presets;
}

bool ProjectMRenderer::isRendering() const {
  return getStatus() == Status::RENDERING;
}


int ProjectMRenderer::getClearRequestedPresetID() {
  std::lock_guard<std::mutex> l(flags_m);
  int r = requestedPresetID;
  requestedPresetID = kPresetIDKeep;
  return r;
}

bool ProjectMRenderer::getClearRequestedToggleAutoplay() {
  std::lock_guard<std::mutex> l(flags_m);
  bool r = requestedToggleAutoplay;
  requestedToggleAutoplay = false;
  return r;
}

ProjectMRenderer::Status ProjectMRenderer::getStatus() const {
  std::lock_guard<std::mutex> l(flags_m);
  return status;
}

void ProjectMRenderer::setStatus(Status s) {
  std::lock_guard<std::mutex> l(flags_m);
  status = s;
}

void ProjectMRenderer::renderSetAutoplay(bool enable) {
  if (pm) {
    std::lock_guard<std::mutex> l(pm_m);
    projectm_lock_preset(pm,!enable);
  }
}

// Switch to the next preset. This should be called only from the
// render thread.
void ProjectMRenderer::renderLoopNextPreset() {
  if (pm) {
    std::lock_guard<std::mutex> l(pm_m);
    unsigned int n = projectm_get_playlist_size(pm);
    if (n) {
      projectm_select_preset(pm,rand() % n,true);
      while (projectm_get_error_loading_current_preset(pm))
        projectm_select_preset(pm,rand() % n,true);
    }
  }
}

// Switch to the indicated preset. This should be called only from
// the render thread.
void ProjectMRenderer::renderLoopSetPreset(unsigned int i) {
  std::lock_guard<std::mutex> l(pm_m);
  unsigned int n = projectm_get_playlist_size(pm);
  if (n && i < n) {
    projectm_select_preset(pm,i,true);
    while (projectm_get_error_loading_current_preset(pm))
      projectm_select_preset(pm,i,true);
  }
}

void ProjectMRenderer::CheckViewportSize(GLFWwindow* win)
{
    int _renderWidth=0;
    int _renderHeight=0;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);
    int tmp1 = windowWidth;
    int tmp2 = windowHeight;
    
    glfwGetFramebufferSize(win, &_renderWidth, &_renderHeight);
    if (renderWidth != _renderWidth || renderHeight != _renderHeight)
    {
        {
        projectm_set_window_size(pm, _renderWidth, _renderHeight);
        //projectm_set_window_size(pm, _renderWidth+RACK_GRID_WIDTH, _renderHeight+20);
        }
        renderWidth = _renderWidth;
        renderHeight = _renderHeight;
        //DEBUG("Resized rendering canvas to %d %d.", renderWidth, renderHeight);
    }
}

void ProjectMRenderer::renderLoop(mySettings s,std::string url,bool windowed ) {
  if (!window) {
    setStatus(Status::FAILED);
    return;
  }

  glfwMakeContextCurrent(window);
  logContextInfo("LowFatMilk window", window);
  
  // Initialize projectM
  projectm_settings *sp = projectm_alloc_settings();
    sp->preset_path = (char *)url.c_str();
    sp->window_width = s.window_width;
    sp->window_height = s.window_height;
    sp->fps =  s.fps;
    sp->mesh_x = s.mesh_x;
    sp->mesh_y = s.mesh_y;
    sp->aspect_correction = s.aspect_correction;

    // Preset display settings
    sp->preset_duration = s.preset_duration;
    sp->soft_cut_duration = s.soft_cut_duration;
    sp->hard_cut_enabled = s.hard_cut_enabled;
    sp->hard_cut_duration= s.hard_cut_duration;
    sp->hard_cut_sensitivity =  s.hard_cut_sensitivity;
    sp->beat_sensitivity = s.beat_sensitivity;
    sp->shuffle_enabled = s.shuffle_enabled;
  {
    std::lock_guard<std::mutex> l(pm_m);
    //DEBUG("The preset path is %s", sp->preset_url);
    pm = projectm_create_settings(sp, PROJECTM_FLAG_NONE);
    CheckViewportSize(window);
  }
  if (pm) {
    setStatus(Status::RENDERING);
    renderLoopNextPreset();
    projectm_select_preset(pm,s.presetIndex,true);
    GLuint FramebufferName = 0;
    GLuint texture = 0;
    glGenFramebuffers(1, &FramebufferName);

    if (windowed)
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    else
      glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

    glGenTextures(1, &texture);
    
    while (true) {
      {
        // Did the main thread request that we exit?
        if (getStatus() == Status::PLEASE_EXIT) {
	  break;
        }
      
    CheckViewportSize(window);

		glBindTexture(GL_TEXTURE_2D, texture);
    
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderWidth,renderHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

        {

    setPresetTime(presetTime);
    setBeatSensitivity(beatSensitivity);
    setHardcutSensitivity(hardcutSensitivity);
    setHardcutDuration(hardcutDuration);
    setSoftcutDuration(softcutDuration);
    setAspectCorrection(aspectCorrection);
    setHardcut(hardCut);

    if (nextPreset) {
      selectNextPreset(projectm_get_hard_cut_enabled(pm));

      nextPreset=false;
    }
    if (prevPreset) {
      selectPreviousPreset(projectm_get_hard_cut_enabled(pm));
      prevPreset=false;
    }
    
	  // Did the main thread request an autoplay toggle?
	  if (getClearRequestedToggleAutoplay()) {
	    renderSetAutoplay(!isAutoplayEnabled());
	  }
	
	  // Did the main thread request that we change the preset?
	  int rpid = getClearRequestedPresetID();
	  if (rpid != kPresetIDKeep) {
	    if (rpid == kPresetIDRandom) {
	      renderLoopNextPreset();
	    } else {
	      renderLoopSetPreset(rpid);
	    }
	  }
        }
      
        {
      	  std::lock_guard<std::mutex> l(pm_m);
          //glViewport(0,0,renderWidth,renderHeight);

	        projectm_render_frame(pm);
              
          GLsizei nrChannels = 4;
          GLsizei stride = nrChannels * renderWidth;
          stride += (stride % 4) ? (4 - stride % 4) : 0;
          bufferSize = stride * renderHeight;

          buffer.reserve(bufferSize);

          glPixelStorei(GL_PACK_ALIGNMENT, 4); 

          glReadPixels(0, 0, renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
      }
        glfwSwapBuffers(window);
      }

      std::this_thread::sleep_for(std::chrono::microseconds(1000000/60));
    }
  }

  {
    std::lock_guard<std::mutex> l(pm_m);
    pm = nullptr;
  }
  glFinish(); // Finish any pending OpenGL operations
  setStatus(Status::EXITING);
}

void ProjectMRenderer::logContextInfo(std::string name, GLFWwindow* w) const {
  int major = glfwGetWindowAttrib(w, GLFW_CONTEXT_VERSION_MAJOR);
  int minor = glfwGetWindowAttrib(w, GLFW_CONTEXT_VERSION_MINOR);
  int revision = glfwGetWindowAttrib(w, GLFW_CONTEXT_REVISION);
  int api = glfwGetWindowAttrib(w, GLFW_CLIENT_API);
  //DEBUG("%s context using API %d version %d.%d.%d", name.c_str(), api, major, minor, revision);
}

void ProjectMRenderer::logGLFWError(int errcode, const char* errmsg) {
  //DEBUG("GLFW error %s: %s", std::to_string(errcode).c_str(), errmsg);
}

GLFWwindow* WindowedRenderer::createWindow(int *xpos,int *ypos,int *width,int *height) {
  glfwSetErrorCallback(logGLFWError);
  logContextInfo("gWindow", APP->window->win);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

  
  c = glfwCreateWindow(RENDER_WIDTH, RACK_GRID_HEIGHT, "", NULL, NULL);
  
  if (!c) {
    return nullptr;
  }
  glfwSetWindowUserPointer(c, reinterpret_cast<void*>(this));
  glfwSetWindowCloseCallback(c, [](GLFWwindow* w) { glfwIconifyWindow(w); });
  glfwSetKeyCallback(c, keyCallback);
  glfwSetWindowPosCallback(c, window_pos_callback);
  glfwSetWindowSizeCallback(c, window_size_callback);
  glfwSetWindowTitle(c, u8"LowFatMilk");
  return c;
}

void WindowedRenderer::window_size_callback(GLFWwindow* win, int width, int height) {
  WindowedRenderer* r = reinterpret_cast<WindowedRenderer*>(glfwGetWindowUserPointer(win));
  *(r->winWidth)=width;
  *(r->winHeight)=height;
}

void WindowedRenderer::window_pos_callback(GLFWwindow* win, int xpos, int ypos) {
  WindowedRenderer* r = reinterpret_cast<WindowedRenderer*>(glfwGetWindowUserPointer(win));
  *(r->xPos)=xpos;
  *(r->yPos)=ypos;
}

void WindowedRenderer::showWindow(int *xpos, int *ypos, int *width, int *height) {
  xPos = xpos;
  yPos = ypos;
  winWidth = width;
  winHeight = height;
  glfwSetWindowPos(c,*(xpos),*(ypos));
  glfwSetWindowSize(c,*(width),*(height));
  glfwShowWindow(c);
}

void WindowedRenderer::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
  WindowedRenderer* r = reinterpret_cast<WindowedRenderer*>(glfwGetWindowUserPointer(win));
  if (action != GLFW_PRESS) return;
  switch (key) {
  case GLFW_KEY_F:
  case GLFW_KEY_F4:
  case GLFW_KEY_ENTER:
    {
      const GLFWmonitor* current_monitor = glfwGetWindowMonitor(win);
      if (!current_monitor) {
	GLFWmonitor* best_monitor = glfwWindowGetNearestMonitor(win);
	const GLFWvidmode* mode = glfwGetVideoMode(best_monitor);
	glfwGetWindowPos(win, &r->last_xpos, &r->last_ypos);
	glfwGetWindowSize(win, &r->last_width, &r->last_height);
	glfwSetWindowMonitor(win, best_monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      } else {
	glfwSetWindowMonitor(win, nullptr, r->last_xpos, r->last_ypos, r->last_width, r->last_height, GLFW_DONT_CARE);
      }

    }
    break;
  case GLFW_KEY_ESCAPE:
  case GLFW_KEY_Q:
    {
      const GLFWmonitor* monitor = glfwGetWindowMonitor(win);
      if (!monitor) {
	glfwIconifyWindow(win);
      } else {
	glfwSetWindowMonitor(win, nullptr, r->last_xpos, r->last_ypos, r->last_width, r->last_height, GLFW_DONT_CARE);
      }
    }
    break;
  case GLFW_KEY_R:
    r->requestPresetID(kPresetIDRandom);
    break;
  default:
    break;
  }
}

GLFWwindow* TextureRenderer::createWindow(int *xpos,int *ypos,int *width,int *height) {
  glfwSetErrorCallback(logGLFWError);
  logContextInfo("gWindow", APP->window->win);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  #if defined ARCH_MAC
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
  #endif
  
  GLFWwindow* c = glfwCreateWindow(RENDER_WIDTH, RACK_GRID_HEIGHT, "", NULL, NULL);

  if (!c) {
    return nullptr;
  }
  glfwSetWindowUserPointer(c, reinterpret_cast<void*>(this));
  logContextInfo("LFM context", c);
  return c;
}

unsigned char* TextureRenderer::getBuffer() {
  return buffer.data();
}

int TextureRenderer::getWindowWidth() {
  return windowWidth;
}

int TextureRenderer::getRenderWidth() {
  return renderWidth;
}

int TextureRenderer::getBufferSize() {
  return bufferSize;
}
