# Makefile for Ace of Penguins Games (Qt Port)
# This can be called from parent Makefile or used standalone

# Qt5 Configuration
QT5_CFLAGS = $(shell pkg-config --cflags Qt5Widgets Qt5Gui Qt5Core Qt5Xml)
QT5_LIBS = $(shell pkg-config --libs Qt5Widgets Qt5Gui Qt5Core Qt5Xml)

# Qt Resource Compiler
RCC = rcc
CXX = g++
CXXFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -O2 -g -I../lib -std=c++11 $(QT5_CFLAGS)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -O2 -g -I../lib $(QT5_CFLAGS)

# Library
LIBRARY = ../lib/libaceqt.a

# Games
LAUNCHER = ace_launcher
SOLITAIRE = solitaire_qt
GOLF = golf_qt
CANFIELD = canfield_qt
SPIDER = spider_qt
FREECELL = freecell_qt
THORNQ = thornq_qt
MASTERMIND = mastermind_qt
MINESWEEPER = minesweeper_qt
PEGGED = pegged_qt
MERLIN = merlin_qt
TAIPEI = taipei_qt
QTAIPEDIT = qtaipedit_qt

# Default target - build all games
all: $(LAUNCHER) $(SOLITAIRE) $(GOLF) $(CANFIELD) $(SPIDER) $(FREECELL) $(THORNQ) $(MASTERMIND) $(MINESWEEPER) $(PEGGED) $(MERLIN) $(TAIPEI) $(QTAIPEDIT)

# Ensure library is built
$(LIBRARY):
	$(MAKE) -C ../lib

# Build launcher - Game selection GUI
$(LAUNCHER): ace_launcher.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ ace_launcher.cpp -L../lib -laceqt $(QT5_LIBS)
	@echo "Built: $@"

# Build solitaire (with embedded resources)
qrc_solitaire.cpp: solitaire.qrc
	$(RCC) -name solitaire solitaire.qrc -o qrc_solitaire.cpp

qrc_solitaire.o: qrc_solitaire.cpp
	$(CXX) $(CXXFLAGS) -c qrc_solitaire.cpp -o qrc_solitaire.o

$(SOLITAIRE): solitaire.c qrc_solitaire.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ solitaire.c qrc_solitaire.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build golf (with embedded resources)
qrc_golf.cpp: golf.qrc
	$(RCC) -name golf golf.qrc -o qrc_golf.cpp

qrc_golf.o: qrc_golf.cpp
	$(CXX) $(CXXFLAGS) -c qrc_golf.cpp -o qrc_golf.o

$(GOLF): golf.c qrc_golf.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ golf.c qrc_golf.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build canfield (with embedded resources)
qrc_canfield.cpp: canfield.qrc
	$(RCC) -name canfield canfield.qrc -o qrc_canfield.cpp

qrc_canfield.o: qrc_canfield.cpp
	$(CXX) $(CXXFLAGS) -c qrc_canfield.cpp -o qrc_canfield.o

$(CANFIELD): canfield.c qrc_canfield.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ canfield.c qrc_canfield.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build spider (with embedded resources)
qrc_spider.cpp: spider.qrc
	$(RCC) -name spider spider.qrc -o qrc_spider.cpp

qrc_spider.o: qrc_spider.cpp
	$(CXX) $(CXXFLAGS) -c qrc_spider.cpp -o qrc_spider.o

$(SPIDER): spider.c qrc_spider.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ spider.c qrc_spider.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build freecell (with embedded resources)
qrc_freecell.cpp: freecell.qrc
	$(RCC) -name freecell freecell.qrc -o qrc_freecell.cpp

qrc_freecell.o: qrc_freecell.cpp
	$(CXX) $(CXXFLAGS) -c qrc_freecell.cpp -o qrc_freecell.o

$(FREECELL): freecell.c qrc_freecell.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ freecell.c qrc_freecell.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build thornq (with embedded resources)
qrc_thornq.cpp: thornq.qrc
	$(RCC) -name thornq thornq.qrc -o qrc_thornq.cpp

qrc_thornq.o: qrc_thornq.cpp
	$(CXX) $(CXXFLAGS) -c qrc_thornq.cpp -o qrc_thornq.o

$(THORNQ): thornq.c qrc_thornq.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ thornq.c qrc_thornq.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build mastermind (with embedded resources)
qrc_mastermind.cpp: mastermind.qrc
	$(RCC) -name mastermind mastermind.qrc -o qrc_mastermind.cpp

qrc_mastermind.o: qrc_mastermind.cpp
	$(CXX) $(CXXFLAGS) -c qrc_mastermind.cpp -o qrc_mastermind.o

$(MASTERMIND): mastermind.c qrc_mastermind.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ mastermind.c qrc_mastermind.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build minesweeper (with embedded resources)
qrc_minesweeper.cpp: minesweeper.qrc
	$(RCC) -name minesweeper minesweeper.qrc -o qrc_minesweeper.cpp

qrc_minesweeper.o: qrc_minesweeper.cpp
	$(CXX) $(CXXFLAGS) -c qrc_minesweeper.cpp -o qrc_minesweeper.o

$(MINESWEEPER): minesweeper.c qrc_minesweeper.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ minesweeper.c qrc_minesweeper.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build pegged (with embedded resources)
qrc_pegged.cpp: pegged.qrc
	$(RCC) -name pegged pegged.qrc -o qrc_pegged.cpp

qrc_pegged.o: qrc_pegged.cpp
	$(CXX) $(CXXFLAGS) -c qrc_pegged.cpp -o qrc_pegged.o

$(PEGGED): pegged.c qrc_pegged.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ pegged.c qrc_pegged.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build merlin (with embedded resources)
qrc_merlin.cpp: merlin.qrc
	$(RCC) -name merlin merlin.qrc -o qrc_merlin.cpp

qrc_merlin.o: qrc_merlin.cpp
	$(CXX) $(CXXFLAGS) -c qrc_merlin.cpp -o qrc_merlin.o

$(MERLIN): merlin.c qrc_merlin.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ merlin.c qrc_merlin.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build taipei (with embedded resources)
qrc_taipei.cpp: taipei.qrc
	$(RCC) -name taipei taipei.qrc -o qrc_taipei.cpp

qrc_taipei.o: qrc_taipei.cpp
	$(CXX) $(CXXFLAGS) -c qrc_taipei.cpp -o qrc_taipei.o

$(TAIPEI): taipei.c taipeilib.c qrc_taipei.o $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ taipei.c taipeilib.c qrc_taipei.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS) -lstdc++
	@echo "Built: $@"

# Build qtaipedit - Taipei layout editor (with embedded resources)
qrc_qtaipedit.cpp: qtaipedit.qrc
	$(RCC) -name qtaipedit qtaipedit.qrc -o qrc_qtaipedit.cpp

qrc_qtaipedit.o: qrc_qtaipedit.cpp
	$(CXX) $(CXXFLAGS) -c qrc_qtaipedit.cpp -o qrc_qtaipedit.o

taipeilib_qtaipedit.o: taipeilib.c
	$(CC) $(CFLAGS) -c taipeilib.c -o taipeilib_qtaipedit.o

$(QTAIPEDIT): qtaipedit.cpp taipeilib_qtaipedit.o qrc_qtaipedit.o $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ qtaipedit.cpp taipeilib_qtaipedit.o qrc_qtaipedit.o -L../lib -laceqt ../lib/qrc_common.o $(QT5_LIBS)
	@echo "Built: $@"

# Clean
clean:
	rm -f $(LAUNCHER) $(SOLITAIRE) $(GOLF) $(CANFIELD) $(SPIDER) $(FREECELL) $(THORNQ) $(MASTERMIND) $(MINESWEEPER) $(PEGGED) $(MERLIN) $(TAIPEI) $(QTAIPEDIT)
	rm -f qrc_*.cpp qrc_*.o taipeilib_qtaipedit.o
	@echo "Cleaned game executables"

# Clean including library
distclean: clean
	$(MAKE) -C ../lib clean

.PHONY: all clean distclean
