cmake_minimum_required(VERSION 2.6)

# Name of the project
project(QuickContour)

# Locate and import ParaView enviroment.
FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

SET (SOURCE_FILES
  qcMainWindow.cxx
  qcMainWindow.h
)

QT4_WRAP_CPP(MOC_SRCS
  qcMainWindow.h
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  qcMainWindow.ui
)

build_paraview_client(quickcontour
  TITLE "Quick Contour (ParaView-based Application)"
  ORGANIZATION "Kitware Inc"
  VERSION_MAJOR 1
  VERSION_MINOR 0
  VERSION_PATCH 0
  PVMAIN_WINDOW qcMainWindow
  PVMAIN_WINDOW_INCLUDE qcMainWindow.h
  SPLASH_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/SplashScreen.png"
  SOURCES ${SOURCE_FILES} ${MOC_SRCS} ${UI_BUILT_SOURCES} ${RCS_BUILT_SOURCES}
)
