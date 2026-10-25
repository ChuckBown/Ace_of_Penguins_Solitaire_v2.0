cmake_minimum_required(VERSION 3.16)
project(AceOfPenguinsQt VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 or Qt6
find_package(Qt5 COMPONENTS Widgets Xml QUIET)
if(NOT Qt5_FOUND)
    find_package(Qt6 COMPONENTS Widgets Xml QUIET)
endif()

if(Qt5_FOUND)
    message(STATUS "Found Qt5: ${Qt5_DIR}")
    set(QT_VERSION_MAJOR 5)
elseif(Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_DIR}")
    set(QT_VERSION_MAJOR 6)
else()
    message(FATAL_ERROR "Qt5 or Qt6 not found. Please install Qt and set CMAKE_PREFIX_PATH to Qt installation directory.")
endif()

# Enable automatic MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/lib
)

# ============================================================================
# Qt Graphics Library (NEW - Qt5 implementation)
# ============================================================================

set(QT_LIB_SOURCES
    lib/qwin.cpp
    lib/qtable.cpp
    lib/qimagelib.cpp
    lib/qhelp.cpp
    lib/qscore.cpp
    lib/funcs.c
    lib/stack.c
)

# Qt Resource files (common resources for all games)
set(QT_LIB_RESOURCES
    lib/resources/common.qrc
)

add_library(acelib_qt STATIC ${QT_LIB_SOURCES} ${QT_LIB_RESOURCES})

if(Qt5_FOUND)
    target_link_libraries(acelib_qt Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(acelib_qt Qt6::Widgets Qt6::Xml)
endif()

# ============================================================================
# Solitaire Game - Qt Port
# ============================================================================

add_executable(solitaire
    games/solitaire.c
    games/solitaire.qrc
    games/solitaire.rc
)

target_link_libraries(solitaire acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(solitaire Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(solitaire Qt6::Widgets Qt6::Xml)
endif()

# On Windows, make it a GUI application (no console window)
# COMMENTED OUT FOR DEVELOPMENT - we want to see debug printf output from stubs
# Uncomment for release builds when debug messages are removed
if(WIN32)
    set_target_properties(solitaire PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Golf Game - Qt Port
# ============================================================================

add_executable(golf
    games/golf.c
    games/golf.qrc
    games/golf.rc
)

target_link_libraries(golf acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(golf Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(golf Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(golf PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Canfield Game - Qt Port
# ============================================================================

add_executable(canfield
    games/canfield.c
    games/canfield.qrc
    games/canfield.rc
)

target_link_libraries(canfield acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(canfield Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(canfield Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(canfield PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Spider Game - Qt Port
# ============================================================================

add_executable(spider
    games/spider.c
    games/spider.qrc
    games/spider.rc
)

target_link_libraries(spider acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(spider Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(spider Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(spider PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Freecell Game - Qt Port
# ============================================================================

add_executable(freecell
    games/freecell.c
    games/freecell.qrc
    games/freecell.rc
)

target_link_libraries(freecell acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(freecell Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(freecell Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(freecell PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Thornq Game - Qt Port
# ============================================================================

add_executable(thornq
    games/thornq.c
    games/thornq.qrc
    games/thornq.rc
)

target_link_libraries(thornq acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(thornq Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(thornq Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(thornq PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Yukon Game - Qt Port
# ============================================================================

add_executable(yukon
    games/yukon.c
    games/yukon.qrc
    games/yukon.rc
)

target_link_libraries(yukon acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(yukon Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(yukon Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(yukon PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Mastermind Game - Qt Port
# ============================================================================

add_executable(mastermind
    games/mastermind.c
    games/mastermind.qrc
    games/mastermind.rc
)

target_link_libraries(mastermind acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(mastermind Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(mastermind Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(mastermind PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Minesweeper Game - Qt Port
# ============================================================================

add_executable(minesweeper
    games/minesweeper.c
    games/minesweeper.qrc
    games/minesweeper.rc
)

target_link_libraries(minesweeper acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(minesweeper Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(minesweeper Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(minesweeper PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Pegged Game - Qt Port
# ============================================================================

add_executable(pegged
    games/pegged.c
    games/pegged.qrc
    games/pegged.rc
)

target_link_libraries(pegged acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(pegged Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(pegged Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(pegged PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Merlin Game - Qt Port
# ============================================================================

add_executable(merlin
    games/merlin.c
    games/merlin.qrc
    games/merlin.rc
)

target_link_libraries(merlin acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(merlin Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(merlin Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(merlin PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Taipei Game - Qt Port
# ============================================================================

add_executable(taipei
    games/taipei.c
    games/taipeilib.c
    games/taipei.qrc
    games/taipei.rc
)

target_link_libraries(taipei acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(taipei Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(taipei Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(taipei PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Qtaipedit - Taipei Layout Editor (Not a game, utility program)
# ============================================================================

add_executable(taipedit
    games/qtaipedit.cpp
    games/taipeilib.c
    games/qtaipedit.qrc
    games/taipedit.rc
)

target_link_libraries(taipedit acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(taipedit Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(taipedit Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(taipedit PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Ace Launcher - Game Selection GUI
# ============================================================================

add_executable(ace_launcher
    games/ace_launcher.cpp
    games/ace_launcher.rc
)

target_link_libraries(ace_launcher acelib_qt)

if(Qt5_FOUND)
    target_link_libraries(ace_launcher Qt5::Widgets Qt5::Xml)
elseif(Qt6_FOUND)
    target_link_libraries(ace_launcher Qt6::Widgets Qt6::Xml)
endif()

if(WIN32)
    set_target_properties(ace_launcher PROPERTIES WIN32_EXECUTABLE ON)
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS ace_launcher solitaire golf canfield spider freecell thornq yukon mastermind minesweeper pegged merlin taipei taipedit
    RUNTIME DESTINATION bin
)

# ============================================================================
# Build Information
# ============================================================================

message(STATUS "===========================================")
message(STATUS "Ace of Penguins - Qt${QT_VERSION_MAJOR} Port")
message(STATUS "===========================================")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "===========================================")
