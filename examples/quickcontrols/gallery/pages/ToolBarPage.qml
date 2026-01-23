// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: page
    enabled: !GalleryConfig.disabled

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: height
            }

            Label {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Header")

                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            ToolSeparator { }

            ToolButton {
                text: "\u2699"
                Accessible.name: qsTr("Settings")
            }
        }
    }

    Label {
        anchors.centerIn: parent
        width: parent.width - 20
        wrapMode: Label.Wrap
        horizontalAlignment: Qt.AlignHCenter
        text: qsTr("ToolBar provides a horizontal container for application-wide "
                 + "and context-sensitive controls, such as navigation buttons and "
                 + "search fields, typically used as a header or footer within an "
                 + "application window")
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent

            Label {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: "\u2139"

                Accessible.name: qsTr("Info")
                Layout.fillHeight: true
                Layout.preferredWidth: height
            }

            Label {
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Footer")

                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            ToolSeparator { }

            ToolButton {
                text: "\u2630"
                Accessible.name: "Hamburger menu"
            }
        }
    }
}
