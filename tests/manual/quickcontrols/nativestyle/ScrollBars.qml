// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
        }

        ScrollBar {
            height: 150
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            property bool qqc2_style_small
        }

        ScrollBar {
            height: 100
            orientation: Qt.Vertical
            size: 0.2
            policy: ScrollBar.AlwaysOn
            property bool qqc2_style_mini
        }

        Column {
            spacing: container.rowSpacing

            ScrollBar {
                width: 300
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
            }

            ScrollBar {
                width: 200
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                property bool qqc2_style_small
            }

            ScrollBar {
                width: 100
                orientation: Qt.Horizontal
                size: 0.2
                policy: ScrollBar.AlwaysOn
                property bool qqc2_style_mini
            }
        }
    }

}
