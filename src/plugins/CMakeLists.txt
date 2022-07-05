# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from plugins.pro.

if(QT_FEATURE_qml_debug AND QT_FEATURE_thread)
    add_subdirectory(qmltooling)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(scenegraph)
endif()

add_subdirectory(qmllint)
