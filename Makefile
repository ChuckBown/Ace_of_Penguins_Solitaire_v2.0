# Makefile for Ace of Penguins Qt Port
# Linux with Qt5

# Qt5 Configuration
QT5_CFLAGS = $(shell pkg-config --cflags Qt5Widgets Qt5Gui Qt5Core Qt5Xml)
QT5_LIBS = $(shell pkg-config --libs Qt5Widgets Qt5Gui Qt5Core Qt5Xml)

# Qt Resource Compiler
RCC = rcc

# Compiler flags
CC = gcc
CXX = g++
CFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -O2 -g -Ilib
CXXFLAGS = -Wall -Wno-unused-function -Wno-unused-variable -O2 -g -Ilib -std=c++11 $(QT5_CFLAGS)

# Library source files
LIB_CPP_SOURCES = lib/qwin.cpp lib/qtable.cpp lib/qimagelib.cpp lib/qhelp.cpp lib/qscore.cpp
LIB_C_SOURCES = lib/funcs.c lib/stack.c
LIB_QRC_SOURCES = lib/resources/common.qrc
LIB_QRC_GENERATED = lib/qrc_common.cpp
LIB_OBJECTS = lib/qwin.o lib/qtable.o lib/qimagelib.o lib/qhelp.o lib/qscore.o \
              lib/funcs.o lib/stack.o lib/qrc_common.o

# Targets
LIBRARY = lib/libaceqt.a
LAUNCHER = ace_launcher
SOLITAIRE = solitaire
GOLF = golf
CANFIELD = canfield
SPIDER = spider
FREECELL = freecell
THORNQ = thornq
YUKON = yukon
MASTERMIND = mastermind
MINESWEEPER = minesweeper
PEGGED = pegged
MERLIN = merlin
TAIPEI = taipei
TAIPEDIT = taipedit

# Default target
all: $(LAUNCHER) $(SOLITAIRE) $(GOLF) $(CANFIELD) $(SPIDER) $(FREECELL) $(THORNQ) $(YUKON) $(MASTERMIND) $(MINESWEEPER) $(PEGGED) $(MERLIN) $(TAIPEI) $(TAIPEDIT)

# Build library
$(LIBRARY): $(LIB_OBJECTS)
	ar rcs $@ $^
	@echo "Library built: $@"
	@echo

# Ace Launcher - Game selection GUI
$(LAUNCHER): games/ace_launcher.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) $(QT5_CFLAGS) -o $@ games/ace_launcher.cpp -L./lib -laceqt $(QT5_LIBS)
	@echo "Built: $@"
	@echo

# Solitaire game (with embedded resources)
games/qrc_solitaire.cpp: games/solitaire.qrc
	$(RCC) -name solitaire games/solitaire.qrc -o games/qrc_solitaire.cpp

games/qrc_solitaire.o: games/qrc_solitaire.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_solitaire.cpp -o games/qrc_solitaire.o

$(SOLITAIRE): games/solitaire.c games/qrc_solitaire.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/solitaire.c games/qrc_solitaire.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Golf game (with embedded resources)
games/qrc_golf.cpp: games/golf.qrc
	$(RCC) -name golf games/golf.qrc -o games/qrc_golf.cpp

games/qrc_golf.o: games/qrc_golf.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_golf.cpp -o games/qrc_golf.o

$(GOLF): games/golf.c games/qrc_golf.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/golf.c games/qrc_golf.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Canfield game (with embedded resources)
games/qrc_canfield.cpp: games/canfield.qrc
	$(RCC) -name canfield games/canfield.qrc -o games/qrc_canfield.cpp

games/qrc_canfield.o: games/qrc_canfield.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_canfield.cpp -o games/qrc_canfield.o

$(CANFIELD): games/canfield.c games/qrc_canfield.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/canfield.c games/qrc_canfield.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Spider game (with embedded resources)
games/qrc_spider.cpp: games/spider.qrc
	$(RCC) -name spider games/spider.qrc -o games/qrc_spider.cpp

games/qrc_spider.o: games/qrc_spider.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_spider.cpp -o games/qrc_spider.o

$(SPIDER): games/spider.c games/qrc_spider.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/spider.c games/qrc_spider.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Freecell game (with embedded resources)
games/qrc_freecell.cpp: games/freecell.qrc
	$(RCC) -name freecell games/freecell.qrc -o games/qrc_freecell.cpp

games/qrc_freecell.o: games/qrc_freecell.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_freecell.cpp -o games/qrc_freecell.o

$(FREECELL): games/freecell.c games/qrc_freecell.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/freecell.c games/qrc_freecell.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Thornq game (with embedded resources)
games/qrc_thornq.cpp: games/thornq.qrc
	$(RCC) -name thornq games/thornq.qrc -o games/qrc_thornq.cpp

games/qrc_thornq.o: games/qrc_thornq.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_thornq.cpp -o games/qrc_thornq.o

$(THORNQ): games/thornq.c games/qrc_thornq.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/thornq.c games/qrc_thornq.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Yukon game (with embedded resources)
games/qrc_yukon.cpp: games/yukon.qrc
	$(RCC) -name yukon games/yukon.qrc -o games/qrc_yukon.cpp

games/qrc_yukon.o: games/qrc_yukon.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_yukon.cpp -o games/qrc_yukon.o

$(YUKON): games/yukon.c games/qrc_yukon.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/yukon.c games/qrc_yukon.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Mastermind game (with embedded resources)
games/qrc_mastermind.cpp: games/mastermind.qrc
	$(RCC) -name mastermind games/mastermind.qrc -o games/qrc_mastermind.cpp

games/qrc_mastermind.o: games/qrc_mastermind.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_mastermind.cpp -o games/qrc_mastermind.o

$(MASTERMIND): games/mastermind.c games/qrc_mastermind.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/mastermind.c games/qrc_mastermind.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Minesweeper game (with embedded resources)
games/qrc_minesweeper.cpp: games/minesweeper.qrc
	$(RCC) -name minesweeper games/minesweeper.qrc -o games/qrc_minesweeper.cpp

games/qrc_minesweeper.o: games/qrc_minesweeper.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_minesweeper.cpp -o games/qrc_minesweeper.o

$(MINESWEEPER): games/minesweeper.c games/qrc_minesweeper.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/minesweeper.c games/qrc_minesweeper.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Pegged game (with embedded resources)
games/qrc_pegged.cpp: games/pegged.qrc
	$(RCC) -name pegged games/pegged.qrc -o games/qrc_pegged.cpp

games/qrc_pegged.o: games/qrc_pegged.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_pegged.cpp -o games/qrc_pegged.o

$(PEGGED): games/pegged.c games/qrc_pegged.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/pegged.c games/qrc_pegged.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Merlin game (with embedded resources)
games/qrc_merlin.cpp: games/merlin.qrc
	$(RCC) -name merlin games/merlin.qrc -o games/qrc_merlin.cpp

games/qrc_merlin.o: games/qrc_merlin.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_merlin.cpp -o games/qrc_merlin.o

$(MERLIN): games/merlin.c games/qrc_merlin.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/merlin.c games/qrc_merlin.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Taipei game (with embedded resources)
games/qrc_taipei.cpp: games/taipei.qrc
	$(RCC) -name taipei games/taipei.qrc -o games/qrc_taipei.cpp

games/qrc_taipei.o: games/qrc_taipei.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_taipei.cpp -o games/qrc_taipei.o

$(TAIPEI): games/taipei.c games/taipeilib.c games/qrc_taipei.o $(LIBRARY)
	$(CC) $(CFLAGS) $(QT5_CFLAGS) -o $@ games/taipei.c games/taipeilib.c games/qrc_taipei.o -L./lib -laceqt $(QT5_LIBS) -lstdc++
	@echo "Built: $@"
	@echo

# Taipedit - Taipei layout editor (with embedded resources)
games/qrc_qtaipedit.cpp: games/qtaipedit.qrc
	$(RCC) -name qtaipedit games/qtaipedit.qrc -o games/qrc_qtaipedit.cpp

games/qrc_qtaipedit.o: games/qrc_qtaipedit.cpp
	$(CXX) $(CXXFLAGS) -c games/qrc_qtaipedit.cpp -o games/qrc_qtaipedit.o

games/taipeilib_qtaipedit.o: games/taipeilib.c
	$(CC) $(CFLAGS) -c games/taipeilib.c -o games/taipeilib_qtaipedit.o

$(TAIPEDIT): games/qtaipedit.cpp games/taipeilib_qtaipedit.o games/qrc_qtaipedit.o $(LIBRARY)
	$(CXX) $(CXXFLAGS) $(QT5_CFLAGS) -o $@ games/qtaipedit.cpp games/taipeilib_qtaipedit.o games/qrc_qtaipedit.o -L./lib -laceqt $(QT5_LIBS)
	@echo "Built: $@"

# C++ object files
lib/qwin.o: lib/qwin.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib/qtable.o: lib/qtable.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib/qimagelib.o: lib/qimagelib.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib/qhelp.o: lib/qhelp.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

lib/qscore.o: lib/qscore.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Qt Resource files
lib/qrc_common.cpp: $(LIB_QRC_SOURCES)
	$(RCC) -name common $(LIB_QRC_SOURCES) -o lib/qrc_common.cpp

lib/qrc_common.o: lib/qrc_common.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# C object files
lib/funcs.o: lib/funcs.c
	$(CC) $(CFLAGS) -c $< -o $@

lib/stack.o: lib/stack.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(LIB_OBJECTS) $(LIBRARY) $(LIB_QRC_GENERATED)
	rm -f $(LAUNCHER) $(SOLITAIRE) $(GOLF) $(CANFIELD) $(SPIDER) $(FREECELL) $(THORNQ) $(YUKON) $(MASTERMIND) $(MINESWEEPER) $(PEGGED) $(MERLIN) $(TAIPEI) $(TAIPEDIT)
	rm -f games/qrc_*.cpp games/qrc_*.o games/taipeilib_qtaipedit.o
	@echo "Cleaned build files"

# Rebuild everything
rebuild: clean all

# Help
help:
	@echo "Ace of Penguins Qt Port - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build all games (solitaire, golf, canfield, spider, freecell, thornq, mastermind, minesweeper, pegged, merlin, taipei)"
	@echo "  make all      - Same as 'make'"
	@echo "  make clean    - Remove all build files"
	@echo "  make rebuild  - Clean and rebuild"
	@echo ""
	@echo "Requirements:"
	@echo "  - Qt5 development packages (qt5-default or qtbase5-dev)"
	@echo "  - pkg-config"
	@echo "  - gcc and g++"
	@echo ""
	@echo "To install Qt5 on Ubuntu/Debian:"
	@echo "  sudo apt-get install qtbase5-dev qt5-qmake"

.PHONY: all clean rebuild help
