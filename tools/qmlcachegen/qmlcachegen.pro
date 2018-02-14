option(host_build)

QT = qmldevtools-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

SOURCES = qmlcachegen.cpp \
    resourcefilter.cpp \
    generateloader.cpp \
    resourcefilemapper.cpp
TARGET = qmlcachegen

build_integration.files = qmlcache.prf qtquickcompiler.prf
build_integration.path = $$[QT_HOST_DATA]/mkspecs/features
prefix_build: INSTALLS += build_integration
else: COPIES += build_integration

cmake_build_integration.files = Qt5QuickCompilerConfig.cmake
cmake_build_integration.path = $$[QT_INSTALL_LIBS]/cmake/Qt5QuickCompiler
prefix_build: INSTALLS += cmake_build_integration
else: COPIES += cmake_build_integration

QMAKE_TARGET_DESCRIPTION = QML Cache Generator

load(qt_tool)

HEADERS += \
    resourcefilemapper.h
