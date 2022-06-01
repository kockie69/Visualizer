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
	LDFLAGS += -lopengl32 dep/lib/projectM.dll -shared 
endif

ifdef ARCH_LIN
	LDFLAGS += -lopengl32 dep/lib/libprojectM.so -shared 

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
//DISTRIBUTABLES += $(wildcard res/*.svg)
DISTRIBUTABLES += $(wildcard LICENSE*) res
//DISTRIBUTABLES += $(wildcard LICENSE*)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk