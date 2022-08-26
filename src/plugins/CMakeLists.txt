# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from plugins.pro.

if(QT_FEATURE_qml_debug AND QT_FEATURE_thread)
    add_subdirectory(qmltooling)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(scenegraph)
endif()

add_subdirectory(qmllint)
