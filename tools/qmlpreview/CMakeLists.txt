# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
