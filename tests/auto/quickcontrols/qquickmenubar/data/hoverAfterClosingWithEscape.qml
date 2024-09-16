// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 300
    height: 300

    property alias fileMenu: fileMenu

    menuBar: MenuBar {
        objectName: "menuBar"

        Menu {
            id: fileMenu
            title: qsTr("File")

            MenuItem {
                text: qsTr("New")
            }
        }

        Menu {
            title: qsTr("Edit")

            MenuItem {
                text: qsTr("Undo")
            }
        }

        Menu {
            title: qsTr("View")

            MenuItem {
                text: qsTr("Center")
            }
        }

        Menu {
            title: qsTr("Tools")

            MenuItem {
                text: qsTr("Options")
            }
        }
    }
}
