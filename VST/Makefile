VERSION = 0.6.1dev

RACK_DIR ?= ../..
include $(RACK_DIR)/arch.mk

VST2_SDK ?= VST2_SDK
TARGET := VCV-Bridge

FLAGS += -fPIC
FLAGS += -I$(VST2_SDK) -I$(RACK_DIR)/include

ifeq ($(ARCH), mac)
	FLAGS += -arch i386 -arch x86_64
	LDFLAGS += -arch i386 -arch x86_64
	LDFLAGS += -bundle
	BUNDLE := $(TARGET).vst
endif
ifeq ($(ARCH), win)
	LDFLAGS += -shared -static -lws2_32
	TARGET := $(TARGET).dll
endif
ifeq ($(ARCH), lin)
$(error The VST plugin is not available for Linux at this time. If you've built Linux VSTs before, you're welcome to contribute to the build system)
endif


SOURCES += BridgeVST.cpp
SOURCES += $(VST2_SDK)/public.sdk/source/vst2.x/audioeffect.cpp
SOURCES += $(VST2_SDK)/public.sdk/source/vst2.x/audioeffectx.cpp
SOURCES += $(VST2_SDK)/public.sdk/source/vst2.x/vstplugmain.cpp


all: $(TARGET)

include $(RACK_DIR)/compile.mk

clean:
	rm -rfv build $(TARGET) dist

dist: all
	rm -rf dist
ifeq ($(ARCH), mac)
	mkdir -p dist
	mkdir -p dist/$(BUNDLE)/Contents
	mkdir -p dist/$(BUNDLE)/Contents/MacOS
	cp Info.plist dist/$(BUNDLE)/Contents/Info.plist
	cp PkgInfo dist/$(BUNDLE)/Contents/
	cp $(TARGET) dist/$(BUNDLE)/Contents/MacOS/
	strip -S dist/$(BUNDLE)/Contents/MacOS/$(TARGET)
	cd dist && zip -5 -r VCV-Bridge-$(VERSION)-$(ARCH)-VST.zip $(BUNDLE)
endif
ifeq ($(ARCH), win)
	mkdir -p dist
	cp $(TARGET) dist/
	strip -s dist/$(TARGET)
	cd dist && zip -5 -r VCV-Bridge-$(VERSION)-$(ARCH)-VST.zip $(TARGET)
endif

install: dist
ifeq ($(ARCH), mac)
	sudo cp -R dist/$(BUNDLE) /Library/Audio/Plug-Ins/VST/
endif
ifeq ($(ARCH), win)
	mkdir -p /c/Program\ Files/Steinberg/VstPlugins
	cp -R dist/$(TARGET) /c/Program\ Files/Steinberg/VstPlugins/
endif

uninstall:
ifeq ($(ARCH), mac)
	sudo rm -rfv /Library/Audio/Plug-Ins/VST/$(BUNDLE)
endif

run: install
ifeq ($(ARCH), win)
	/c/Program\ Files/REAPER\ \(x64\)/reaper
endif
