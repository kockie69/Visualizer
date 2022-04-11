# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS += 
FLAGS += 
CFLAGS += /mingw64/include/
CXXFLAGS += 

include $(RACK_DIR)/arch.mk

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.

ifdef ARCH_WIN
	LDFLAGS += ./dep/lib/liblibprojectM.a /mingw64/lib/libopengl32.a /mingw64/lib/libgomp.a
endif

ifdef ARCH_WIN
	LDLIBS += -lopengl32  -lpthread -mthreads -pthread
endif

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp) 

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
# DISTRIBUTABLES += $(wildcard res/*.svg)
DISTRIBUTABLES += $(wildcard LICENSE*) res
DISTRIBUTABLES += $(wildcard LICENSE*)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

