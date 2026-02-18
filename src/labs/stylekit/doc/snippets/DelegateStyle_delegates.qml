// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit
import QtQuick.Templates as T

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {

        //! [data]
        component OverlayData : QtObject {
            property color overlayColor
        }

        toolButton {
            background.delegate: StyledItem {
                id: custom
                Text {
                    color: custom.delegateStyle.data.overlayColor
                    font.pixelSize: 30
                    text: "シ"
                }
            }
            background.data: OverlayData {
                overlayColor: "sandybrown"
            }
            hovered.background.data: OverlayData {
                overlayColor: "magenta"
            }
        }
        //! [data]

        //! [delegate]
        // import QtQuick.Templates as T

        slider {
            handle.delegate: Rectangle {
                id: handle
                required property DelegateStyle delegateStyle
                required property T.Slider control
                implicitWidth: delegateStyle.implicitWidth
                implicitHeight: delegateStyle.implicitHeight
                radius: delegateStyle.radius
                color: delegateStyle.color
                Text {
                    anchors.centerIn: parent
                    text: handle.control.value.toFixed(0)
                }
            }
        }
        //! [delegate]
    }

    // The rest of the file is not a part of the docs. It just implements a small
    // UI to allow testing the style from the command line using the 'qml' app.

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            ToolBar {
                id: tb
                Row {
                    spacing: tb.spacing
                    anchors.fill: parent
                    ToolButton { text: "Tool 1" }
                    ToolButton { text: "Tool 2" }
                    ToolSeparator { anchors.verticalCenter: parent.verticalCenter }
                    ToolButton { text: "Tool 3" }
                }
            }

            Slider {
                width: 200
            }
        }
    }
}
