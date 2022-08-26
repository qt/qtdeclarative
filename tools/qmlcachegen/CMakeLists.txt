# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlcachegen.pro.

#####################################################################
## qmlcachegen Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlcachegen)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Cache Generator"
    TOOLS_TARGET Qml # special case
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        qmlcachegen.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::QmlCompilerPrivate
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qmlcachegen.pro:<TRUE>:
# CMAKE_BIN_DIR = "$$cmakeRelativePath($$[QT_HOST_BINS], $$[QT_INSTALL_PREFIX])"
# QMAKE_SUBSTITUTES = "cmake_config_file"
# QMAKE_TARGET_DESCRIPTION = "QML" "Cache" "Generator"
# _OPTION = "host_build"
# build_integration.files = "qmlcache.prf" "qtquickcompiler.prf"
# build_integration.path = "$$[QT_HOST_DATA]/mkspecs/features"
# cmake_build_integration.files = "$$cmake_config_file.output"
# cmake_build_integration.path = "$$[QT_INSTALL_LIBS]/cmake/Qt6QuickCompiler"
# cmake_config_file.input = "$$PWD/Qt6QuickCompilerConfig.cmake.in"
# cmake_config_file.output = "$$MODULE_BASE_OUTDIR/lib/cmake/Qt6QuickCompiler/Qt6QuickCompilerConfig.cmake"

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

# special case begin
# Install public prf files.
set(qmlcachegen_mkspecs
    "${CMAKE_CURRENT_SOURCE_DIR}/qmlcache.prf"
    "${CMAKE_CURRENT_SOURCE_DIR}/qtquickcompiler.prf"
    )
set(mkspecs_install_dir "${INSTALL_MKSPECSDIR}")
qt_path_join(mkspecs_install_dir "${QT_INSTALL_DIR}" "${mkspecs_install_dir}" "features")
qt_copy_or_install(FILES "${qmlcachegen_mkspecs}"
                   DESTINATION ${mkspecs_install_dir})
# special case end
