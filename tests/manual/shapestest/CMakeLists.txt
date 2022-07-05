# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from shapestest.pro.

#####################################################################
## shapestest Binary:
#####################################################################

qt_internal_add_manual_test(shapestest
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(shapestest_resource_files
    "shapestest.qml"
)

qt_internal_add_resource(shapestest "shapestest"
    PREFIX
        "/shapestest"
    FILES
        ${shapestest_resource_files}
)


#### Keys ignored in scope 1:.:.:shapestest.pro:<TRUE>:
# OTHER_FILES = "shapestest.qml"
# TEMPLATE = "app"
