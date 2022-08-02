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
	projectm := dep/projectm/build/src/libprojectM/liblibprojectM.a
	glew := dep/lib/libglew32.a
endif

ifdef ARCH_LIN
	LDFLAGS += -fopenmp -shared
	projectm := dep/projectm/build/src/libprojectM/libprojectM.a
endif

ifdef ARCH_MAC
#	LDFLAGS += src/dep/projectm/lib/libprojectM.4.dylib -shared
	projectm := dep/projectm/build/src/libprojectM/libprojectM.a
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
DEPS += $(glew)

glew-2.1.0:
	cd dep && $(WGET) "https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0.tgz"
	cd dep && $(SHA256) glew-2.1.0.tgz 04de91e7e6763039bc11940095cd9c7f880baba82196a7765f727ac05a993c95
	cd dep && $(UNTAR) glew-2.1.0.tgz
	cd dep && rm glew-2.1.0.tgz

$(glew): | glew-2.1.0
	cd dep/glew-2.1.0 && mkdir -p build
	cd dep/glew-2.1.0/build && $(CMAKE) ./cmake
	cd dep && $(MAKE) -C glew-2.1.0/build
	cd dep && $(MAKE) -C glew-2.1.0/build install

$(projectm): | $(glew)
	# Out-of-source build dir
	cd dep && git submodule update --init
	sh checkout_older.sh "$(ARCH_WIN)"
	cd dep/projectm && mkdir -p build
	cd dep/projectm/build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL="OFF" -DCMAKE_INSTALL_PREFIX=../../../dep/ ..
	sh update_cache.sh "$(ARCH_WIN)"
	cd dep/projectm/build && cmake --build .
	cd dep/projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
