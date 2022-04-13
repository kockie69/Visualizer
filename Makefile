# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

include $(RACK_DIR)/arch.mk

# FLAGS will be passed to both the C and C++ compiler

ifdef ARCH_WIN
	FLAGS += -D_USE_MATH_DEFINES -DprojectM_main_EXPORTS
endif

ifdef ARCH_LIN
	LDFLAGS += ./dep/lib/libprojectM.so
endif

FLAGS += 
CFLAGS += /mingw64/include/
CXXFLAGS += 

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.

ifdef ARCH_WIN
	#LDFLAGS += ./dep/lib/liblibprojectM.a /mingw64/lib/libopengl32.a /mingw64/lib/libgomp.a
	LDFLAGS += ./dep/lib/liblibprojectM.a -lpsapi /mingw64/lib/libopengl32.a /mingw64/lib/libgomp.a -lpthread -mthreads -pthread
endif

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
# DISTRIBUTABLES += $(wildcard res/*.svg)
DISTRIBUTABLES += $(wildcard LICENSE*) res
DISTRIBUTABLES += $(wildcard LICENSE*)

# Define the path of the built static library
projectm := dep/lib/liblibprojectM.a
# Build the static library into your plugin.dll/dylib/so
OBJECTS += $(projectm)
# Trigger the static library to be built when running `make dep`
DEPS += $(projectm)

$(projectm):
	# Out-of-source build dir
	# check /d
	ls /d
	# check /a
	ls/d/a
	cd src/deps/projectm && mkdir -p build
	cd src/deps/projectm/build && $(CMAKE) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=D:/msys64/home/rober/Visualizer/dep ..
	# Install to dep/
	cd src/deps/projectm/build && $(MAKE) install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk