option(host_build)

QT = core qmldevtools-private
DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

SOURCES += main.cpp
include(../shared/shared.pri)

load(cmake_functions)

CMAKE_BIN_DIR = $$cmakeRelativePath($$[QT_HOST_BINS], $$[QT_INSTALL_PREFIX])
contains(CMAKE_BIN_DIR, "^\\.\\./.*") {
    CMAKE_BIN_DIR = $$[QT_HOST_BINS]/
    CMAKE_BIN_DIR_IS_ABSOLUTE = True
}

CMAKE_QML_DIR = $$cmakeRelativePath($$[QT_INSTALL_QML], $$[QT_INSTALL_PREFIX])
contains(CMAKE_QML_DIR, "^\\.\\./.*") {
    CMAKE_QML_DIR = $$[QT_INSTALL_QML]/
    CMAKE_QML_DIR_IS_ABSOLUTE = True
}
load(qt_build_paths)

static|staticlib:CMAKE_STATIC_TYPE = true

# Compute the platform target suffix.
CMAKE_QML_PLUGIN_SUFFIX_RELEASE =
win32: CMAKE_QML_PLUGIN_SUFFIX_DEBUG = d
else:darwin: CMAKE_QML_PLUGIN_SUFFIX_DEBUG = _debug
else: CMAKE_QML_PLUGIN_SUFFIX_DEBUG =

# Find out which configurations should be handled in the generated Config.cmake file.
CMAKE_DEBUG_TYPE =
CMAKE_RELEASE_TYPE =
if(qtConfig(debug_and_release)|contains(QT_CONFIG, debug, debug|release)): CMAKE_DEBUG_TYPE = debug
if(qtConfig(debug_and_release)|contains(QT_CONFIG, release, debug|release)): CMAKE_RELEASE_TYPE = release

qtConfig(debug_and_release) {
    CMAKE_DEBUG_AND_RELEASE = TRUE
} else {
    CMAKE_DEBUG_AND_RELEASE = FALSE
}

equals(QMAKE_HOST.os, Windows): CMAKE_BIN_SUFFIX = ".exe"
cmake_config_file.input = $$PWD/Qt5QmlImportScannerConfig.cmake.in
cmake_config_file.output = $$MODULE_BASE_OUTDIR/lib/cmake/Qt5QmlImportScanner/Qt5QmlImportScannerConfig.cmake
QMAKE_SUBSTITUTES += cmake_config_file

cmake_build_integration.files = $$cmake_config_file.output $$PWD/Qt5QmlImportScannerTemplate.cpp.in
cmake_build_integration.path = $$[QT_INSTALL_LIBS]/cmake/Qt5QmlImportScanner
prefix_build: INSTALLS += cmake_build_integration
else: COPIES += cmake_build_integration

QMAKE_TARGET_DESCRIPTION = QML Import Scanner

load(qt_tool)
