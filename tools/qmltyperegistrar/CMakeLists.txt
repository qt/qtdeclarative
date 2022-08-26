# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmltyperegistrar Tool:
#####################################################################

qt_get_tool_target_name(target_name qmltyperegistrar)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Types Registrar"
    TOOLS_TARGET Qml # special case
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        qmltyperegistrar.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Core
        Qt::QmlTypeRegistrarPrivate
)

# support for .pro projects needing qmltyperegistrar
set(qmltyperegistrar_mkspecs "${CMAKE_CURRENT_SOURCE_DIR}/qmltypes.prf")
set(mkspecs_install_dir "${INSTALL_MKSPECSDIR}")
qt_path_join(mkspecs_install_dir "${QT_INSTALL_DIR}" "${mkspecs_install_dir}" "features")
qt_copy_or_install(FILES "${qmltyperegistrar_mkspecs}"
                   DESTINATION ${mkspecs_install_dir})
