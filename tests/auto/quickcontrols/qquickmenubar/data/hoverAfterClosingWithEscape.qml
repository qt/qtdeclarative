// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 300
    height: 300

    MenuBar {
        objectName: "menuBar"

        Menu {
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
