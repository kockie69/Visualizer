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
	projectm := ./dep/lib/liblibprojectM.a
endif

ifdef ARCH_LIN
	LDFLAGS += -fopenmp -shared
	projectm := ./dep/lib/libprojectM.a
endif

ifdef ARCH_MAC
#	LDFLAGS += src/dep/projectm/lib/libprojectM.4.dylib -shared
	projectm := ./dep/lib/libprojectM.a
endif


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
	# Out-of-source build dir
	cd src && mkdir -p dep
	cd src/dep && rm -fr projectm
	cd src/dep && git clone https://github.com/projectM-visualizer/projectm.git 
	cd src/dep/projectm && git fetch --all --tags
	cd src/dep/projectm && git checkout 4ad0242c60720ea9d334baec810936e672a3154e
	cd src/dep/projectm && mkdir -p build
	cd src/dep/projectm/build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL=OFF -DCMAKE_INSTALL_PREFIX=../../../../dep/ ..
	sh update_cache.sh "$(ARCH_WIN)"
	cd src/dep/projectm/build && cmake --build .
	cd src/dep/projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk