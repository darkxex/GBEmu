# SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
# SPDX-License-Identifier: MIT

SHELL     := /bin/sh
CXX       := g++
PREFIX    := /usr/local
TARGET    := gb.exe
BUILD     := build

CXXFLAGS  := -std=c++23 -Wall -Wextra -Wno-unused-parameter -MMD -MP -O3 -DNDEBUG \
             -DPATCH=0x$(shell git rev-parse --short HEAD 2>/dev/null || echo 0)
INCLUDES  := $(shell find src -type d | sed "s/^/-I/")
CPPFLAGS  := $(shell pkg-config --cflags sdl3)
LDLIBS    := $(shell pkg-config --libs sdl3)

SOURCES   := $(shell find src -name "*.cpp")
OBJECTS   := $(addprefix $(BUILD)/,$(SOURCES:.cpp=.o))
DEPS      := $(addprefix $(BUILD)/,$(SOURCES:.cpp=.d))

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD) $(TARGET)

.PHONY: format
format:
	clang-format -i src/*.cpp src/*.h

.PHONY: install
install:
	install -D -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)
