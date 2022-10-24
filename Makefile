# If RACK_DIR is not defined when calling the Makefile, default to two directories above
$(info Rackdir is $(RACK_DIR))
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
	LDFLAGS += -lopengl32
	projectm := dep/lib/liblibprojectM.a
else	
	projectm := dep/lib/libprojectM.a
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
#DEPS += $(glew)

$(projectm):
# 	Out-of-source build dir
	cd dep && git submodule update --init

# There is an issue with the latest version for projectm on windows so we need to rollback a version
# These lines can be removed when ProjectM fix #632 is deployed	

	cd dep/projectm && git checkout --force a6293f63c8415cc757f89b82dcc99738d0c83027

# I built my own api service but it is not part of ProjectM yet
# So copy my files over the existing to make the api available.

	cp src/dep/ProjectM.cpp dep/projectm/src/libprojectM
	cp src/dep/projectM.h dep/projectm/src/libprojectM
	cp src/dep/ProjectM.hpp dep/projectm/src/libprojectM
	cp src/dep/ProjectMCWrapper.cpp dep/projectm/src/libprojectM

# Start building
	cd dep/projectm && mkdir -p build

# Config make customization per platform type
# An additional lib needs to be added for the build of projectm, so sed to the rescue
ifdef ARCH_WIN
	cp src/dep/CMakeLists.txt dep/projectm
	cd dep/projectm/build && cmake -G "Ninja" -DCMAKE_LIBRARY_PATH=dep/lib -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_SDL="OFF" -DCMAKE_INSTALL_PREFIX=../../../dep/ ..
	cp $(RACK_DIR)/libRack.dll.a dep/
	sed -i 's/CMAKE_CXX_STANDARD_LIBRARIES:STRING=/CMAKE_CXX_STANDARD_LIBRARIES:STRING= ..\/..\/..\/dep\/libRack.dll.a -lpsapi /g' dep/projectm/build/CMakeCache.txt; 
else
	cd dep/projectm/build && cmake -DCMAKE_LIBRARY_PATH=dep/lib -DENABLE_OPENMP="OFF" -DCMAKE_BUILD_TYPE=Release -DENABLE_THREADING="OFF" -DENABLE_SDL="OFF" -DCMAKE_INSTALL_PREFIX=../../../dep/ ..
endif
	
	cd dep/projectm/build && cmake --build .
	cd dep/projectm/build && cmake --build . --target install

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
