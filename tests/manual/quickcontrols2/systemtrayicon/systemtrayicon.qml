// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import Qt.labs.platform

ApplicationWindow {
    id: window

    visible: true
    title: "Qt Quick Controls 2 - System Tray Icon"

    MenuBar {
        id: menuBar

        Menu {
            id: fileMenu
            title: qsTr("File")

            MenuItem {
                id : quitItem
                text : "Quit"
                onTriggered: Qt.quit()
            }
        }
    }

    SystemTrayIcon {
        visible: true
        icon.source: "qrc:/files/images/qt_logo_green_256.png"

        menu: Menu {
            MenuItem {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }

        onActivated: console.log("Activated")
    }
}
