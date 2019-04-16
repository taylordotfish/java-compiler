BUILD_DIR = build

# Set to 1 for debug mode.
DEBUG = 1

# Set to 1 to keep assertions in non-debug mode.
ASSERTIONS = 0

BINARY = compiler
SOURCES = $(shell find src -type f -name '*.cpp')



CXX = g++
CXXFLAGS = \
	-Wall -Wextra -Wno-error -pedantic -std=c++17 -fpic -MMD -MP -Isrc \
	-fvisibility=hidden
LDFLAGS =
LDLIBS =



ifeq ($(DEBUG), 1)
	CXXFLAGS += -Og -ggdb
	ASSERTIONS := 1
else
	CXXFLAGS += -Ofast -flto -fuse-linker-plugin
	LDFLAGS += -Ofast -flto -fuse-linker-plugin
endif

ifneq ($(ASSERTIONS), 1)
	CXXFLAGS += -DNDEBUG
endif



OBJ_DIR = $(BUILD_DIR)/objects
add_build = $(addprefix $(BUILD_DIR)/,$(1))
add_obj = $(addprefix $(OBJ_DIR)/,$(1))
src_to_obj = $(call add_obj,$(addsuffix .o,$(basename $(1))))

BINARY := $(call add_build,$(BINARY))
OBJECTS = $(call src_to_obj,$(SOURCES))

ALL_BINARIES = $(BINARY)
ALL_OBJECTS = $(OBJECTS)
BUILD_SUBDIRS = $(sort $(dir $(ALL_OBJECTS)))



.PHONY: all
all: $(ALL_BINARIES)

$(BINARY): $(OBJECTS)

$(ALL_BINARIES):
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(ALL_OBJECTS:.o=.d)

$(ALL_OBJECTS) $(ALL_BINARIES): | $(BUILD_SUBDIRS)

$(BUILD_SUBDIRS):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean-binaries
clean-binaries:
	rm -rf $(ALL_BINARIES)
