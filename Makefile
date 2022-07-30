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
	LDFLAGS += -lopengl32 -fopenmp -shared 
endif

ifdef ARCH_LIN
	LDFLAGS += -fopenmp -shared 
endif

ifdef ARCH_MAC
#	LDFLAGS += src/dep/projectm/lib/libprojectM.4.dylib -shared
endif


# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += $(wildcard LICENSE*) res 

projectm := ./dep/lib/libprojectM.a

# Build the static library into your plugin.dll/dylib/so
OBJECTS += $(projectm)
# Trigger the static library to be built when running `make dep`
DEPS += $(projectm)

$(projectm):
	# Out-of-source build dir
	cd src && mkdir -p dep
	cd src/dep && rm -fr projectm
	cd src/dep && git clone https://github.com/projectM-visualizer/projectm.git 
	cd src/dep/projectm && git fetch --all --tags
	cd src/dep/projectm && mkdir -p build
	cd src/dep/projectm/build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL=OFF -DCMAKE_INSTALL_PREFIX=../../../../dep/ ..
	sed -i '' -e 's/CMAKE_CXX_STANDARD_LIBRARIES:STRING=/CMAKE_CXX_STANDARD_LIBRARIES:STRING=-lpsapi /g' src/dep/projectm/build/CMakeCache.txt
	cd src/dep/projectm/build && cmake --build .
	cd src/dep/projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk