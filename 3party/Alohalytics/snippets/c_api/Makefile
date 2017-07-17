.PHONY: all indent clean

ALOHALYTICS_ROOT=../../Alohalytics
BUILD_DIR=build

CPP=g++
CFLAGS=-Wall -Werror -O3 -fpic
CPPFLAGS=-std=c++11 $(CFLAGS) -I$(ALOHALYTICS_ROOT)/src

SRC=$(wildcard *.cc)
OBJ=$(SRC:%.cc=$(BUILD_DIR)/%.o)
SO=$(OBJ:%.o=%.so)

all: build $(SO)

indent:
	(find . -name "*.cc" -o -name "*.h") | xargs clang-format -i

clean:
	-rm -rf $(BUILD_DIR)

build:
	mkdir -p $@

$(BUILD_DIR)/%.so: $(notdir %.cc)
	$(CPP) $(CPPFLAGS) -shared -o $@ $<
