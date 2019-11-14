# Generated from qmlcachegen.pro.

#####################################################################
## qmlcachegen Tool:
#####################################################################

qt_add_tool(qmlcachegen
    TOOLS_TARGET Qml # special case
    SOURCES
        generateloader.cpp
        qmlcachegen.cpp
        resourcefilemapper.cpp resourcefilemapper.h
        resourcefilter.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::QmlDevToolsPrivate
    PUBLIC_LIBRARIES
        Qt::QmlDevTools
)

#### Keys ignored in scope 1:.:.:qmlcachegen.pro:<TRUE>:
# CMAKE_BIN_DIR = "$$cmakeRelativePath($$[QT_HOST_BINS], $$[QT_INSTALL_PREFIX])"
# QMAKE_SUBSTITUTES = "cmake_config_file"
# QMAKE_TARGET_DESCRIPTION = "QML" "Cache" "Generator"
# _OPTION = "host_build"
# build_integration.files = "qmlcache.prf" "qtquickcompiler.prf"
# build_integration.path = "$$[QT_HOST_DATA]/mkspecs/features"
# cmake_build_integration.files = "$$cmake_config_file.output"
# cmake_build_integration.path = "$$[QT_INSTALL_LIBS]/cmake/Qt5QuickCompiler"
# cmake_config_file.input = "$$PWD/Qt5QuickCompilerConfig.cmake.in"
# cmake_config_file.output = "$$MODULE_BASE_OUTDIR/lib/cmake/Qt5QuickCompiler/Qt5QuickCompilerConfig.cmake"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmlcachegen.pro:prefix_build:
# INSTALLS = "cmake_build_integration" "build_integration"

#### Keys ignored in scope 3:.:.:qmlcachegen.pro:else:
# COPIES = "cmake_build_integration" "build_integration"

#### Keys ignored in scope 4:.:.:qmlcachegen.pro:CMAKE_BIN_DIR___contains___^\\.\\./._x_:
# CMAKE_BIN_DIR = "$$[QT_HOST_BINS]/"
# CMAKE_BIN_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 5:.:.:qmlcachegen.pro:QMAKE_HOST.os___equals___Windows:
# CMAKE_BIN_SUFFIX = ".exe"
