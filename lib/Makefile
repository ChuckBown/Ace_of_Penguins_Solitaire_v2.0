# Makefile for Ace of Penguins Qt Graphics Library
# This can be called from parent Makefile or used standalone

# Qt5 Configuration
QT5_CFLAGS = $(shell pkg-config --cflags Qt5Widgets Qt5Gui Qt5Core Qt5Xml)
QT5_LIBS = $(shell pkg-config --libs Qt5Widgets Qt5Gui Qt5Core Qt5Xml)

# Qt Resource Compiler
RCC = rcc

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -O2 -g -I.
CXXFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -O2 -g -I. -std=c++11 $(QT5_CFLAGS)

# Source files
CPP_SOURCES = qwin.cpp qtable.cpp qimagelib.cpp qhelp.cpp qscore.cpp
C_SOURCES = funcs.c stack.c
QRC_SOURCES = resources/common.qrc
QRC_GENERATED = qrc_common.cpp
OBJECTS = qwin.o qtable.o qimagelib.o qhelp.o qscore.o funcs.o stack.o qrc_common.o

# Library target
LIBRARY = libaceqt.a

# Default target
all: $(LIBRARY)

# Build static library
$(LIBRARY): $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	@echo "Library built: $@"

# C++ compilation
qwin.o: qwin.cpp
	$(CXX) $(CXXFLAGS) -c qwin.cpp -o qwin.o

qtable.o: qtable.cpp
	$(CXX) $(CXXFLAGS) -c qtable.cpp -o qtable.o

qimagelib.o: qimagelib.cpp
	$(CXX) $(CXXFLAGS) -c qimagelib.cpp -o qimagelib.o

qhelp.o: qhelp.cpp
	$(CXX) $(CXXFLAGS) -c qhelp.cpp -o qhelp.o

qscore.o: qscore.cpp
	$(CXX) $(CXXFLAGS) -c qscore.cpp -o qscore.o

# Qt Resource compilation
qrc_common.cpp: $(QRC_SOURCES)
	$(RCC) -name common $(QRC_SOURCES) -o qrc_common.cpp

qrc_common.o: qrc_common.cpp
	$(CXX) $(CXXFLAGS) -c qrc_common.cpp -o qrc_common.o

# C compilation
funcs.o: funcs.c
	$(CC) $(CFLAGS) -c funcs.c -o funcs.o

stack.o: stack.c
	$(CC) $(CFLAGS) -c stack.c -o stack.o

# Clean
clean:
	rm -f $(OBJECTS) $(LIBRARY) $(QRC_GENERATED)
	@echo "Cleaned library build files"

.PHONY: all clean
