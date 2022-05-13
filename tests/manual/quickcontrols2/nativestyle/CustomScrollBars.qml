// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ControlContainer {
    id: container
    title: "ScrollBars"

    Row {
        spacing: container.rowSpacing

        ScrollBar {
            height: 200
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            background: Rectangle {
                color: "lightgray"
                border.color: "gray"
                border.width: 1
            }
        }

        ScrollBar {
            height: 200
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            contentItem: Rectangle {
                color: "lightgreen"
                border.color: "green"
                border.width: 1
            }
        }

        ScrollBar {
            height: 200
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            background: Rectangle {
                color: "lightgray"
                border.color: "gray"
                border.width: 1
            }
            contentItem: Rectangle {
                implicitWidth: 15
                color: "lightgreen"
                border.color: "green"
                border.width: 1
            }
        }

        Column {
            spacing: container.rowSpacing

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                background: Rectangle {
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 1
                }
            }

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                contentItem: Rectangle {
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 1
                }
            }

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                background: Rectangle {
                    color: "lightgray"
                    border.color: "gray"
                    border.width: 1
                }
                contentItem: Rectangle {
                    implicitHeight: 15
                    color: "lightgreen"
                    border.color: "green"
                    border.width: 1
                }
            }
        }
    }

}
