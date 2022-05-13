// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 300
    height: 300
    visible: true
    MenuBar {
        id: mb
        objectName: "menuBar"
        width: parent.width
        Menu {
            title: "StaticMenu"
            MenuItem {
                text: "Cut"
            }
            MenuItem {
                text: "Copy"
            }
            MenuItem {
                text: "Paste"
            }
        }
    }
    Component {
        id: cmp
        Menu {
            title: "DynamicMenu"
            MenuItem {
                text: "Cut"
            }
            MenuItem {
                text: "Copy"
            }
            MenuItem {
                text: "Paste"
            }
        }
    }
    Component.onCompleted: {
        mb.addMenu(cmp.createObject(mb))
    }
}
