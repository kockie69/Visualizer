#define NANOVG_GL2
#include "RPJ.hpp"
#include "Renderer.hpp"
#include "GLFW/glfw3.h"
#include "GlfwUtils.hpp"
#include <thread>
#include <mutex>

// Create ModulePresetPathItems for each patch in a directory.
void ProjectMRenderer::loadPresetItems(std::string presetDir) {

	if (system::isDirectory(presetDir)) {
		// Note: This is not cached, so opening this each time might have a bit of latency.
		std::vector<std::string> entries = system::getEntries(presetDir,5);
		std::sort(entries.begin(), entries.end());
		for (std::string path : entries) {
			if (!system::isDirectory(path)) {
        fullList.push_back(path);
		  }
	  } 
  }
}

void ProjectMRenderer::init(mySettings const& s,int *xpos, int *ypos,int *width,int *height,bool windowed,bool alwaysOnTop,bool noFrames) {
  window = createWindow(xpos,ypos,width,height,alwaysOnTop,noFrames);
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

void ProjectMRenderer::setNoFrames(bool noFrames) {
  if (noFrames) {
    glfwSetWindowAttrib(window,GLFW_DECORATED,!noFrames);
    glfwSetWindowPosCallback(window, window_pos_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
  }
  else {
    glfwSetWindowAttrib(window,GLFW_DECORATED,!noFrames);
    glfwSetWindowPosCallback(window, NULL);
    glfwSetWindowSizeCallback(window, NULL);
    glfwSetMouseButtonCallback(window, NULL);
    glfwSetCursorPosCallback(window, NULL);
  }
}

void ProjectMRenderer::cursor_position_callback(GLFWwindow* win, double x, double y){
    ProjectMRenderer* r = reinterpret_cast<ProjectMRenderer*>(glfwGetWindowUserPointer(win));
    if (r->buttonEvent == 1) {
        r->offset_cpx = x - r->cp_x;
        r->offset_cpy = y - r->cp_y;
    }
}

void ProjectMRenderer::mouse_button_callback(GLFWwindow* win, int button, int action, int mods){
  ProjectMRenderer* r = reinterpret_cast<ProjectMRenderer*>(glfwGetWindowUserPointer(win));
  if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
    r->buttonEvent = 1;
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    r->cp_x = floor(x);
    r->cp_y= floor(y);
  }
  if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
      r->buttonEvent = 0;
      r->cp_x = 0;
      r->cp_y = 0;
  }
}

void ProjectMRenderer::window_size_callback(GLFWwindow* win, int width, int height) {
  ProjectMRenderer* r = reinterpret_cast<ProjectMRenderer*>(glfwGetWindowUserPointer(win));
  *(r->winWidth)=width;
  *(r->winHeight)=height;
}

void ProjectMRenderer::window_pos_callback(GLFWwindow* win, int xpos, int ypos) {
  ProjectMRenderer* r = reinterpret_cast<ProjectMRenderer*>(glfwGetWindowUserPointer(win));
  *(r->xPos)=xpos;
  *(r->yPos)=ypos;
}

void ProjectMRenderer::setAlwaysOnTop(bool alwaysOnTop) {
  glfwSetWindowAttrib(window,GLFW_FLOATING,alwaysOnTop);
}

void ProjectMRenderer::addPCMData(float* data, unsigned int nsamples) {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_pcm_add_float(pm,data,nsamples,PROJECTM_STEREO);
}

// Requests that projectM changes the preset at the next opportunity
void ProjectMRenderer::requestPresetID(int id) {
  std::lock_guard<std::mutex> l(flags_m);
  if (id==-1) {
    if (fullList.size()) 
      newPresetName=fullList[rand() % fullList.size()].data();
  }
  else
    newPresetName=fullList[id].data();
}

// Requests that projectM changes the preset at the next opportunity
void ProjectMRenderer::requestPresetName(std::string preset_name, bool hard_cut) {
  std::lock_guard<std::mutex> l(pm_m);
  if (pm) {
    projectm_load_preset_file(pm,preset_name.c_str(),!hard_cut);
    presetNameActive = preset_name;
    switchPreset = false;
  }
}

// Requests that projectM changes the preset at the next opportunity
void ProjectMRenderer::setRequestPresetName(std::string preset_name) {
  newPresetName = preset_name;
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
  return !(projectm_get_preset_locked(pm));
}

// Switches to the previous preset in the current playlist.
void ProjectMRenderer::selectPreviousPreset(bool hard_cut) {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  auto it = std::find(fullList.begin(),fullList.end(),presetNameActive);
  if (it != fullList.end()) { // Found the title
    it--;
    newPresetName=*it;
  }
  else {
    it = fullList.end();
    it--;
    newPresetName=*it;
  }
}

// Switches to the next preset in the current playlist.
void ProjectMRenderer::selectNextPreset(bool hard_cut) {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  auto it = std::find(fullList.begin(), fullList.end(), presetNameActive);
    if (it != fullList.end()) { // Found the title
      it++;
      if (it != fullList.end()) {
        newPresetName = *it;
      }
      else {
        it = fullList.begin();
        newPresetName = *it;
      }
    }
}

// ID of the current preset in projectM's list
unsigned int ProjectMRenderer::activePreset() const {
  unsigned int presetIdx;
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return 0;

  return presetIdx;
}

// Name of the preset projectM is currently displaying
std::string ProjectMRenderer::activePresetName() {
  return presetNameActive;
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

void ProjectMRenderer::PresetSwitchedEvent(bool isHardCut, void* context)
{
    auto that = reinterpret_cast<ProjectMRenderer*>(context);
    that->switchPreset = true;
}

void ProjectMRenderer::PresetSwitchedErrorEvent(const char* preset_filename,const char* message, void* user_data)
{
    auto that = reinterpret_cast<ProjectMRenderer*>(user_data);
    that->newPresetName = that->activePresetName();
}


// Returns a list of all presets currently loaded by projectM
std::list<std::pair<unsigned int, std::string> > ProjectMRenderer::listPresets() const {
  std::list<std::pair<unsigned int, std::string> > presets;
  
  //DEBUG("Ok, lets check the number of presets found");
  unsigned int n;
  n = fullList.size();

  //DEBUG("The number of presets found is %d", n);
  if (!n) {
    return presets;
  }
  for (unsigned int i = 0; i < n; ++i){
    std::string s;
    {
      s = fullList[i];
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
    projectm_set_preset_locked(pm,!enable);
  }
}

// Switch to the next preset. This should be called only from the
// render thread.
void ProjectMRenderer::renderLoopNextPreset() {
  if (pm) {
    std::lock_guard<std::mutex> l(pm_m);
    
    unsigned int n = fullList.size();
    if (n) {
      int index = rand() % n;
      newPresetName = fullList[index].data();
    }
  }
}

// Switch to the indicated preset. This should be called only from
// the render thread.
void ProjectMRenderer::renderLoopSetPreset(unsigned int i) {
  std::lock_guard<std::mutex> l(pm_m);
}

void ProjectMRenderer::CheckViewportSize(GLFWwindow* win)
{
    int _renderWidth=0;
    int _renderHeight=0;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);
    
    glfwGetFramebufferSize(win, &_renderWidth, &_renderHeight);
    if (renderWidth != _renderWidth || renderHeight != _renderHeight)
    {
        {
        projectm_set_window_size(pm, _renderWidth, _renderHeight);
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
    mySettings *sp = new mySettings();
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
    loadPresetItems((char *)url.c_str());
  {
    std::lock_guard<std::mutex> l(pm_m);
    //DEBUG("The preset path is %s", sp->preset_url);
    pm = projectm_create();
    CheckViewportSize(window);
  }
  if (pm) {
    setStatus(Status::RENDERING);
    projectm_set_preset_switch_requested_event_callback(pm, &ProjectMRenderer::PresetSwitchedEvent,static_cast<void*>(this));
    projectm_set_preset_switch_failed_event_callback(pm, &ProjectMRenderer::PresetSwitchedErrorEvent,static_cast<void*>(this));                                                

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
    if(buttonEvent == 1){
            glfwGetWindowPos(window, &w_posx, &w_posy);
            glfwSetWindowPos(window, w_posx + offset_cpx, w_posy + offset_cpy);
            offset_cpx = 0;
            offset_cpy = 0;
            cp_x += offset_cpx;
            cp_y += offset_cpy;
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

    if (newPresetName != "") {
      requestPresetName(newPresetName,hardCut);
      newPresetName = "";
    }
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

	        projectm_render_frame(pm);
          if (!windowed) {    
            GLsizei nrChannels = 4;
            GLsizei stride = nrChannels * renderWidth;
            stride += (stride % 4) ? (4 - stride % 4) : 0;
            bufferSize = stride * renderHeight;

            buffer.reserve(bufferSize);

            glPixelStorei(GL_PACK_ALIGNMENT, 4); 

            glReadPixels(0, 0, renderWidth, renderHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
          }
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

GLFWwindow* WindowedRenderer::createWindow(int *xpos,int *ypos,int *width,int *height,bool alwaysOnTop,bool noFrames) {
  glfwSetErrorCallback(logGLFWError);
  logContextInfo("gWindow", APP->window->win);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  if (alwaysOnTop)
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
  if (noFrames)
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

  
  c = glfwCreateWindow(RENDER_WIDTH, RACK_GRID_HEIGHT, "", NULL, NULL);
  
  if (!c) {
    return nullptr;
  }
  glfwSetWindowUserPointer(c, reinterpret_cast<WindowedRenderer*>(this));
  glfwSetWindowCloseCallback(c, [](GLFWwindow* w) { glfwIconifyWindow(w); });
  glfwSetKeyCallback(c, keyCallback);
  if (noFrames) {
    glfwSetWindowPosCallback(c, window_pos_callback);
    glfwSetWindowSizeCallback(c, window_size_callback);
    glfwSetMouseButtonCallback(c, mouse_button_callback);
    glfwSetCursorPosCallback(c, cursor_position_callback);
  }
  glfwSetWindowTitle(c, u8"LowFatMilk");
  return c;
}


void WindowedRenderer::showWindow(int *xpos, int *ypos, int *width, int *height) {
  xPos = xpos;
  yPos = ypos;
  winWidth = width;
  winHeight = height;
  glfwSetWindowPos(c,*(xpos),*(ypos));
  glfwSetWindowSize(c,*(width),*(height));
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

GLFWwindow* TextureRenderer::createWindow(int *xpos,int *ypos,int *width,int *height,bool unused1,bool unused2) {
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
  
  c = glfwCreateWindow(RENDER_WIDTH, RACK_GRID_HEIGHT, "", NULL, NULL);

  if (!c) {
    return nullptr;
  }
  glfwSetWindowUserPointer(c, reinterpret_cast<TextureRenderer*>(this));
  logContextInfo("LFM context", c);
  return c;
}

void TextureRenderer::showWindow(int *xpos, int *ypos, int *width, int *height) {
  glfwSetWindowSize(c,*(width),380);
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
