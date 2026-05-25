# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

# Install public prf files.
set(qmlcachegen_mkspecs
    "${CMAKE_CURRENT_SOURCE_DIR}/qmlcache.prf"
    "${CMAKE_CURRENT_SOURCE_DIR}/qtquickcompiler.prf"
    )
set(mkspecs_install_dir "${INSTALL_MKSPECSDIR}")
qt_path_join(mkspecs_install_dir "${QT_INSTALL_DIR}" "${mkspecs_install_dir}" "features")
qt_copy_or_install(FILES "${qmlcachegen_mkspecs}"
                   DESTINATION ${mkspecs_install_dir})

qt_internal_return_unless_building_tools()
