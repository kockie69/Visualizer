# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

include $(RACK_DIR)/arch.mk

# FLAGS will be passed to both the C and C++ compiler

ifdef ARCH_WIN
	FLAGS += -D_USE_MATH_DEFINES -DprojectM_main_EXPORTS 
endif

#ifdef ARCH_LIN
#	LDFLAGS += ./dep/lib/Linux/libprojectM.so
#endif

FLAGS += 
CFLAGS += /mingw64/include/
CXXFLAGS += 

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.

ifdef ARCH_WIN
	LDFLAGS += -lopengl32 ~/Visualizer/dep/lib/projectM.dll fopenmp -shared 
endif

ifdef ARCH_LIN
	LDFLAGS += ~/Visualizer/dep/lib/libprojectM.so.4 -fopenmp -shared 
endif

ifdef ARCH_MAC
	LDFLAGS += ~/Visualizer/dep/lib/libprojectM.4.dylib -fopenmp -shared 
endif

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
ifdef ARCH_WIN
	DISTRIBUTABLES += $(wildcard LICENSE*) res ~/Visualizer/dep/lib/projectM.dll
endif

ifdef ARCH_LIN
	DISTRIBUTABLES += $(wildcard LICENSE*) res ~/Visualizer/dep/lib/libprojectM.so.4 ~/Visualizer/dep/lib/libprojectM.so.4.0.0
endif

ifdef ARCH_MAC
	DISTRIBUTABLES += $(wildcard LICENSE*) res ~/Visualizer/dep/lib/libprojectM.4.dylib
endif

# Define the path of the built static library
projectm := ~/Visualizer/dep/lib/libprojectM.a
# Build the static library into your plugin.dll/dylib/so
OBJECTS += $(projectm)
# Trigger the static library to be built when running `make dep`
DEPS += $(projectm)

$(projectm):
	# Out-of-source build dir
	cd .. && rm -fr projectm
	cd .. && git clone https://github.com/projectM-visualizer/projectm.git 
	cd ../projectm && git fetch --all --tags
	cd ../projectm && mkdir -p build
	cd ../projectm/build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL=OFF -DCMAKE_INSTALL_PREFIX=~/Visualizer/dep ..
	cd ../projectm/build && cmake --build .
	cd ../projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk