# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

include $(RACK_DIR)/arch.mk

# FLAGS will be passed to both the C and C++ compiler

ifdef ARCH_WIN
	FLAGS += -ID:/a/Visualizer/Visualizer/src/include/ -D_USE_MATH_DEFINES -DprojectM_api_EXPORTS 
endif

FLAGS += -I./dep/projectm/build/include -ID:/a/Visualizer/Visualizer/src/include/
CFLAGS += /mingw64/include/
CXXFLAGS += 

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.

ifdef ARCH_WIN
	LDFLAGS += -lopengl32
endif
	projectm := dep/projectm/build/lib/libprojectM.a

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res

# Build the static library into your plugin.dll/dylib/so
OBJECTS += $(projectm)
# Trigger the static library to be built when running `make dep`
DEPS += $(projectm)


$(projectm):
# 	Out-of-source build dir
	cd dep && git submodule update --init
	cd dep && git submodule update --remote

# Start building
	cd dep/projectm && mkdir -p build
#	cp src/dep/CMakeLists.txt dep/projectm

# Config make customization per platform type
# An additional lib needs to be added for the build of projectm, so sed to the rescue
ifdef ARCH_WIN
	cp src/dep/FileScanner.cpp dep/projectm/src/libprojectM/Renderer/
	cd dep/projectm/build && cmake -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SHARED_LIBS="OFF" D_FILE_OFFSET_BITS=64 -DCMAKE_CXX_STANDARD_LIBRARIES=-lpsapi -DENABLE_OPENMP="OFF" -DENABLE_THREADING="OFF" -DENABLE_PLAYLIST="OFF" -DCMAKE_INSTALL_PREFIX=. ..
	cp $(RACK_DIR)/libRack.dll.a dep/
else
	cd dep/projectm/build && cmake -DBUILD_SHARED_LIBS="OFF" -DENABLE_SDL_UI="OFF" -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_PLAYLIST="OFF" -DCMAKE_INSTALL_PREFIX=. ..
endif
	
	cd dep/projectm/build && cmake --build .
	cd dep/projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
