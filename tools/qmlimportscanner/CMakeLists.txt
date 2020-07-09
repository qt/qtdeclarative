# Generated from qmlimportscanner.pro.

#####################################################################
## qmlimportscanner Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlimportscanner)
qt_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Import Scanner"
    TOOLS_TARGET Qml # special case
    SOURCES
        ../shared/resourcefilemapper.cpp ../shared/resourcefilemapper.h
        main.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmlimportscanner.pro:<TRUE>:
# CMAKE_BIN_DIR = "$$cmakeRelativePath($$[QT_HOST_BINS], $$[QT_INSTALL_PREFIX])"
# CMAKE_DEBUG_TYPE = <EMPTY>
# CMAKE_QML_DIR = "$$cmakeRelativePath($$[QT_INSTALL_QML], $$[QT_INSTALL_PREFIX])"
# CMAKE_QML_PLUGIN_SUFFIX_RELEASE = <EMPTY>
# CMAKE_RELEASE_TYPE = <EMPTY>
# QMAKE_SUBSTITUTES = "cmake_config_file"
# QMAKE_TARGET_DESCRIPTION = "QML" "Import" "Scanner"
# _OPTION = "host_build"
# cmake_build_integration.files = "$$cmake_config_file.output" "$$PWD/Qt5QmlImportScannerTemplate.cpp.in"
# cmake_build_integration.path = "$$[QT_INSTALL_LIBS]/cmake/Qt5QmlImportScanner"
# cmake_config_file.input = "$$PWD/Qt5QmlImportScannerConfig.cmake.in"
# cmake_config_file.output = "$$MODULE_BASE_OUTDIR/lib/cmake/Qt5QmlImportScanner/Qt5QmlImportScannerConfig.cmake"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qmlimportscanner.pro:CMAKE_BIN_DIR___contains___^\\.\\./._x_:
# CMAKE_BIN_DIR = "$$[QT_HOST_BINS]/"
# CMAKE_BIN_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 3:.:.:qmlimportscanner.pro:CMAKE_QML_DIR___contains___^\\.\\./._x_:
# CMAKE_QML_DIR = "$$[QT_INSTALL_QML]/"
# CMAKE_QML_DIR_IS_ABSOLUTE = "True"

#### Keys ignored in scope 4:.:.:qmlimportscanner.pro:static OR staticlib:
# CMAKE_STATIC_TYPE = "true"

#### Keys ignored in scope 5:.:.:qmlimportscanner.pro:WIN32:
# CMAKE_QML_PLUGIN_SUFFIX_DEBUG = "d"

#### Keys ignored in scope 7:.:.:qmlimportscanner.pro:APPLE:
# CMAKE_QML_PLUGIN_SUFFIX_DEBUG = "_debug"

#### Keys ignored in scope 8:.:.:qmlimportscanner.pro:else:
# CMAKE_QML_PLUGIN_SUFFIX_DEBUG = <EMPTY>

#### Keys ignored in scope 9:.:.:qmlimportscanner.pro:( QT_FEATURE_debug_and_release OR QT_CONFIG___contains___debug,debug OR release ):
# CMAKE_DEBUG_TYPE = "debug"

#### Keys ignored in scope 10:.:.:qmlimportscanner.pro:( QT_FEATURE_debug_and_release OR QT_CONFIG___contains___release,debug OR release ):
# CMAKE_RELEASE_TYPE = "release"

#### Keys ignored in scope 11:.:.:qmlimportscanner.pro:QT_FEATURE_debug_and_release:
# CMAKE_DEBUG_AND_RELEASE = "TRUE"

#### Keys ignored in scope 12:.:.:qmlimportscanner.pro:else:
# CMAKE_DEBUG_AND_RELEASE = "FALSE"

#### Keys ignored in scope 13:.:.:qmlimportscanner.pro:QMAKE_HOST.os___equals___Windows:
# CMAKE_BIN_SUFFIX = ".exe"

#### Keys ignored in scope 14:.:.:qmlimportscanner.pro:prefix_build:
# INSTALLS = "cmake_build_integration"

#### Keys ignored in scope 15:.:.:qmlimportscanner.pro:else:
# COPIES = "cmake_build_integration"
