# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from listview.pro.

#####################################################################
## listview Binary:
#####################################################################

qt_internal_add_manual_test(listview
    GUI
    SOURCES
        main.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
    LIBRARIES
        Qt::Gui
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(listview "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:listview.pro:<TRUE>:
# QML_DESIGNER_IMPORT_PATH = <EMPTY>
# QML_IMPORT_PATH = <EMPTY>

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:listview.pro:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 4:.:.:listview.pro:UNIX AND NOT ANDROID:
# target.path = "/opt/$${TARGET}/bin"

#### Keys ignored in scope 5:.:.:listview.pro:NOT target.path_ISEMPTY:
# INSTALLS = "target"
