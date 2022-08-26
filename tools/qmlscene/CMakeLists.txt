# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmlscene App:
#####################################################################

qt_internal_add_app(qmlscene
    TARGET_DESCRIPTION "QML Scene Viewer"
    SOURCES
        main.cpp
    DEFINES
        QML_RUNTIME_TESTING
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
)

set_target_properties(qmlscene PROPERTIES WIN32_EXECUTABLE FALSE)

qt_internal_extend_target(qmlscene CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(qmlscene CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
