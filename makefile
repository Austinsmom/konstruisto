# Project details
PROJECT_NAME=Konstruisto
PROJECT_VERSION=$(shell git describe --match "v[0-9]*" --abbrev=0 HEAD)
PROJECT_LAST_COMMIT=$(shell git rev-parse --short HEAD)

ifneq ($(CONFIG), RELEASE)
   CONFIG=DEBUG
endif

BUILD_DESC = $(PROJECT_VERSION)-$(PROJECT_LAST_COMMIT) $(CONFIG)

# Paths and dependencies
SRCDIR := src
OBJDIR := obj
BINDIR := bin
EXTDIR := ext

CC=clang
CXX=clang++
RM_R=rm -rf

ifeq ($(OS), Windows_NT)
	EXTENSION := .exe

	INCLUDES += -I$(EXTDIR)/glfw-3.2.1/include
	LDFLAGS  += -L$(EXTDIR)/glfw-3.2.1/lib

	INCLUDES += -I$(EXTDIR)/glew-2.0.0/include
	LDFLAGS  += -L$(EXTDIR)/glew-2.0.0/lib

	INCLUDES += -I$(EXTDIR)/glm/

	LIBS := -lglfw3 -lglew32 -lopengl32 -lglu32 -lgdi32
else

endif

DEFINES +=-DPROJECT_NAME=\""$(PROJECT_NAME)\"" -DPROJECT_VERSION=\""$(PROJECT_VERSION)\"" -DBUILD_DESC=\""$(BUILD_DESC)\""
CPPFLAGS =-std=c++14 -Wall -Wextra -Werror -Wformat-nonliteral -Winit-self -Wno-nonportable-include-path -DGLEW_STATIC

ifeq ($(CONFIG), DEBUG)
	DEFINES +=-DDEBUG
	CPPFLAGS +=-g
else
	CPPFLAGS +=-O3
endif

CPPFLAGS += $(INCLUDES)

# Targets
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
CPP_FILES := $(call rwildcard,$(SRCDIR),*.cpp)
OBJ_FILES := $(addprefix $(OBJDIR)/,$(subst src/, , $(subst .cpp,.o,$(CPP_FILES))))

$(BINDIR)/$(PROJECT_NAME)$(EXTENSION):  $(OBJ_FILES)
	@mkdir -p $(BINDIR)
	@echo "[LINK] $(CXX) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)"
	@$(CXX) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	@echo "[COMPILE] $< $@"
	@$(CXX) -c -o $@ $< $(CPPFLAGS) $(DEFINES)

clean:
	@echo "Removing $(BINDIR)/$(PROJECT_NAME)$(EXTENSION)..."
	@$(RM) $(BINDIR)/$(PROJECT_NAME)$(EXTENSION)
	@echo "Removing $(OBJDIR)/*..."
	@$(RM_R) ./$(OBJDIR)/*

build: $(BINDIR)/$(PROJECT_NAME)$(EXTENSION)

rebuild: clean build

run:
	@echo
	@cd $(BINDIR); ./$(PROJECT_NAME)$(EXTENSION)

all: clean build run

help:
	@echo $(PROJECT_NAME)
	@echo "Targets: clean, build, rebuild (clean + build), run, all, help"
	@echo "Possible CONFIG= values: DEBUG, RELEASE"