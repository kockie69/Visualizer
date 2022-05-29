#define NANOVG_GL2
#include "RPJ.hpp"
#include "Renderer.hpp"
#include "GLFW/glfw3.h"
#include "../dep/include/libprojectM/projectM.h"
#include "glfwUtils.hpp"
#include <thread>
#include <mutex>

void ProjectMRenderer::init(projectm_settings const& s) {
  window = createWindow();
  std::string url = s.preset_url;
  renderThread = std::thread([this,s,url](){ this->renderLoop(s,url); });
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

void ProjectMRenderer::requestToggleHardcut() {
  std::lock_guard<std::mutex> l(flags_m);
  requestedToggleHardcut = true;
}

// True if projectM is autoplaying presets
bool ProjectMRenderer::isAutoplayEnabled() const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return false;
  return !(projectm_is_preset_locked(pm));
}

// True if projectM has Hardcut enabled
bool ProjectMRenderer::isHardcutEnabled() const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return false;
  return !(projectm_get_hard_cut_enabled(pm));
}

// Switches to the previous preset in the current playlist.
void ProjectMRenderer::selectPreviousPreset(bool hard_cut) const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_select_previous_preset(pm,hard_cut); 
}

// Switches to the next preset in the current playlist.
void ProjectMRenderer::selectNextPreset(bool hard_cut) const {
  std::lock_guard<std::mutex> l(pm_m);
  if (!pm) return;
  projectm_select_next_preset(pm,hard_cut);
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

// Returns a list of all presets currently loaded by projectM
std::list<std::pair<unsigned int, std::string> > ProjectMRenderer::listPresets() const {
  std::list<std::pair<unsigned int, std::string> > presets;
  unsigned int n;
  {
    std::lock_guard<std::mutex> l(pm_m);
    if (!pm) return presets;
    n = projectm_get_playlist_size(pm);
  }
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

bool ProjectMRenderer::getClearRequestedToggleHardcut() {
  std::lock_guard<std::mutex> l(flags_m);
  bool r = requestedToggleHardcut;
  requestedToggleHardcut = false;
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

void ProjectMRenderer::renderSetHardcut(bool enable) {
  std::lock_guard<std::mutex> l(pm_m);
  projectm_set_hard_cut_enabled(pm,enable);
}

// Switch to the next preset. This should be called only from the
// render thread.
void ProjectMRenderer::renderLoopNextPreset() {
  if (pm) {
    std::lock_guard<std::mutex> l(pm_m);
    unsigned int n = projectm_get_playlist_size(pm);
    if (n) {
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
  }
}

void ProjectMRenderer::renderLoop(projectm_settings s,std::string url) {
  if (!window) {
    setStatus(Status::FAILED);
    return;
  }

  glfwMakeContextCurrent(window);
  logContextInfo("Milkrack window", window);
  
  // Initialize projectM
  projectm_settings *sp = projectm_alloc_settings();
    sp->preset_url = (char *)url.c_str();
    sp->window_width = 360;
    sp->window_height = 360;
    sp->fps =  60;
    sp->mesh_x = 220;
    sp->mesh_y = 125;
    sp->aspect_correction = true;

    // Preset display settings
    sp->preset_duration = 30;
    sp->soft_cut_duration = 10;
    sp->hard_cut_enabled = false;
    sp->hard_cut_duration= 20;
    sp->hard_cut_sensitivity =  1.0;
    sp->beat_sensitivity = 1.0;
    sp->shuffle_enabled = false;
  {
    std::lock_guard<std::mutex> l(pm_m);
    pm = projectm_create_settings(sp, PROJECTM_FLAG_NONE);
    
    extraProjectMInitialization();
  }
  if (pm) {
    setStatus(Status::RENDERING);
    renderSetAutoplay(false);
    renderLoopNextPreset();
  
    while (true) {
      {
        // Did the main thread request that we exit?
        if (getStatus() == Status::PLEASE_EXIT) {
	  break;
        }
      
        // Resize?
        if (dirtySize) {
	  int x, y;
	  glfwGetFramebufferSize(window, &x, &y);
	  projectm_set_window_size(pm,x, y);
	  dirtySize = false;
        }
      
        {

    if (getClearRequestedToggleHardcut()) {
      renderSetHardcut(false);
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
        }
        int width, height;
        glfwGetFramebufferSize(this->window, &width, &height);
        GLsizei nrChannels = 4;
        GLsizei stride = nrChannels * width;
        stride += (stride % 4) ? (4 - stride % 4) : 0;
        GLsizei bufferSize = stride * height;

        buffer.reserve(bufferSize);

        glPixelStorei(GL_PACK_ALIGNMENT, 4); 
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
        bufferWidth = width;
        glfwSwapBuffers(window);

      }
      std::this_thread::sleep_for(std::chrono::microseconds(1000000/60));
      //usleep(1000000/60); // TODO fps
    }
  }

  {
    std::lock_guard<std::mutex> l(pm_m);
    delete pm;
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
  //rack::logger::log(rack::logger::DEBUG_LEVEL, "Milkrack/" __FILE__, __LINE__, "%s context using API %d version %d.%d.%d", name.c_str(), api, major, minor, revision);
  DEBUG("%s context using API %d version %d.%d.%d", name.c_str(), api, major, minor, revision);
}

void ProjectMRenderer::logGLFWError(int errcode, const char* errmsg) {
  //rack::logger::log(rack::WARN_LEVEL, "Milkrack/" __FILE__, 0, "GLFW error %s: %s", std::to_string(errcode), errmsg);
  DEBUG("GLFW error %s: %s", std::to_string(errcode), errmsg);
}

GLFWwindow* WindowedRenderer::createWindow() {
  glfwSetErrorCallback(logGLFWError);
  glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  GLFWwindow* c = glfwCreateWindow(360, 360, "", NULL, NULL);  
  if (!c) {
    //rack::logger::log(rack::DEBUG_LEVEL, "Milkrack/" __FILE__, __LINE__, "Milkrack renderLoop could not create a context, bailing.");
    DEBUG("Milkrack/" __FILE__, __LINE__, "Milkrack renderLoop could not create a context, bailing.");
    return nullptr;
  }
  glfwSetWindowUserPointer(c, reinterpret_cast<void*>(this));
  glfwSetFramebufferSizeCallback(c, framebufferSizeCallback);
  glfwSetWindowCloseCallback(c, [](GLFWwindow* w) { glfwIconifyWindow(w); });
  glfwSetKeyCallback(c, keyCallback);
  glfwSetWindowTitle(c, u8"LowFatMilk");
  return c;
}

void WindowedRenderer::framebufferSizeCallback(GLFWwindow* win, int x, int y) {
  WindowedRenderer* r = reinterpret_cast<WindowedRenderer*>(glfwGetWindowUserPointer(win));
  r->dirtySize = true;
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

GLFWwindow* TextureRenderer::createWindow() {
  glfwSetErrorCallback(logGLFWError);
  logContextInfo("gWindow", APP->window->win);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow* c = glfwCreateWindow(360, 360, "", NULL, APP->window->win);
  if (!c) {
    //rack::logger::log(rack::DEBUG_LEVEL, "Milkrack/" __FILE__, __LINE__, "Milkrack renderLoop could not create a context, bailing.");
    return nullptr;
  }
  glfwSetWindowUserPointer(c, reinterpret_cast<void*>(this));
  glfwSetFramebufferSizeCallback(c, framebufferSizeCallback);
  logContextInfo("Milkrack context", c);
  return c;
}

void TextureRenderer::framebufferSizeCallback(GLFWwindow* win, int x, int y) {
  TextureRenderer* r = reinterpret_cast<TextureRenderer*>(glfwGetWindowUserPointer(win));
  r->dirtySize = true;
}

void TextureRenderer::extraProjectMInitialization() {
  if (pm)
    texture = projectm_init_render_to_texture(pm);
}

int TextureRenderer::getTextureID() const {
  return texture;
}

unsigned char* TextureRenderer::getBuffer() {
 return buffer.data();
}

int TextureRenderer::getWindowWidth() {
  return bufferWidth;
}