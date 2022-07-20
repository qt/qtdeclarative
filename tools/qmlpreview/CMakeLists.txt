# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qmlpreview.pro.

#####################################################################
## qmlpreview App:
#####################################################################

qt_internal_add_app(qmlpreview
    TARGET_DESCRIPTION "QML Preview"
    SOURCES
        main.cpp
        qmlpreviewapplication.cpp qmlpreviewapplication.h
        qmlpreviewfilesystemwatcher.cpp qmlpreviewfilesystemwatcher.h
    LIBRARIES
        Qt::Network
        Qt::QmlDebugPrivate
)
qt_internal_return_unless_building_tools()

set_target_properties(qmlpreview PROPERTIES WIN32_EXECUTABLE FALSE)

#### Keys ignored in scope 1:.:.:qmlpreview.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Preview"
