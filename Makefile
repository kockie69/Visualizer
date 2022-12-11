# If RACK_DIR is not defined when calling the Makefile, default to two directories above
$(info Rackdir is $(RACK_DIR))
RACK_DIR ?= ../..

include $(RACK_DIR)/arch.mk

# FLAGS will be passed to both the C and C++ compiler

ifdef ARCH_WIN
	FLAGS += -ID:/a/Visualizer/Visualizer/src/include/ -D_USE_MATH_DEFINES -DprojectM_main_EXPORTS 
endif

#ifdef ARCH_LIN
#	LDFLAGS += ./dep/lib/Linux/libprojectM.so
#endif

FLAGS += -I./src/include -ID:/a/Visualizer/Visualizer/src/include/
CFLAGS += /mingw64/include/
CXXFLAGS += 

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.

ifdef ARCH_WIN
	LDFLAGS += -lopengl32
	projectm := D:/a/Visualizer/Visualizer/src/lib/liblibprojectM.a
else	
	projectm := src/lib/libprojectM.a
endif

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res
# DISTRIBUTABLES += $(wildcard LICENSE*) presets

# Build the static library into your plugin.dll/dylib/so
OBJECTS += $(projectm)
# Trigger the static library to be built when running `make dep`
DEPS += $(projectm)
#DEPS += $(glew)

$(projectm):
# 	Out-of-source build dir
	cd dep && git submodule update --init
#	cd dep && git submodule update --remote

# Start building
	cd dep/projectm && mkdir -p build

# Config make customization per platform type
# An additional lib needs to be added for the build of projectm, so sed to the rescue
ifdef ARCH_WIN
	cp src/dep/CMakeLists.txt dep/projectm
	cd dep/projectm/build && cmake -G "Ninja" -DCMAKE_LIBRARY_PATH=dep/lib -DCMAKE_CXX_STANDARD_LIBRARIES=-lpsapi -DENABLE_SHARED_LIB="OFF" -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_SDL="OFF" -DENABLE_PLAYLIST="OFF" -DCMAKE_INSTALL_PREFIX=../../../src/ ..
	cp $(RACK_DIR)/libRack.dll.a dep/
#	sed -i 's/CMAKE_CXX_STANDARD_LIBRARIES:STRING=/CMAKE_CXX_STANDARD_LIBRARIES:STRING= ..\/..\/..\/dep\/libRack.dll.a -lpsapi /g' dep/projectm/build/CMakeCache.txt; 
else
	cd dep/projectm/build && cmake -DCMAKE_LIBRARY_PATH=dep/lib -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_SDL="OFF" -DENABLE_PLAYLIST="OFF" -DCMAKE_INSTALL_PREFIX=../../../src/ ..
endif
	
	cd dep/projectm/build && cmake --build .
	cd dep/projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
