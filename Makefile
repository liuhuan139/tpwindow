CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?=

PKGS = gtk4 libxml-2.0
CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDLIBS += $(shell pkg-config --libs $(PKGS))

SRC_FILES = \
	src/main.cpp \
	src/core/AdbManager.cpp \
	src/core/CacheManager.cpp \
	src/core/XmlParser.cpp \
	src/models/UiNode.cpp \
	src/models/UiTreeModel.cpp \
	src/ui/MainWindow.cpp \
	src/ui/HierarchyWindow.cpp \
	src/ui/NodeAttributesPanel.cpp

OBJ_FILES = $(SRC_FILES:.cpp=.o)

TARGET = topwindow

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ_FILES) $(TARGET)

.PHONY: all clean
