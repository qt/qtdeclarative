// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Flickable {
    id: flickable
    enabled: !GalleryConfig.disabled
    contentHeight: pane.height

    Pane {
        id: pane
        width: flickable.width
        height: flickable.height * 1.25

        Column {
            id: column
            spacing: 40
            width: parent.width

            CheckBox {
                id: alwaysOnCheckBox
                width: parent.width
                text: qsTr("Always on")
            }

            Label {
                width: parent.width
                wrapMode: Label.Wrap
                horizontalAlignment: Qt.AlignHCenter
                text: qsTr("ScrollBar is an interactive bar that can be used to scroll to a specific position. "
                    + "A scroll bar can be either vertical or horizontal, and can be attached to any Flickable, "
                    + "such as ListView and GridView.")
            }

            Image {
                rotation: 90
                source: "../images/arrows.png"
                anchors.horizontalCenter: parent.horizontalCenter
                Accessible.name: qsTr("Image of an arrow pointing in both vertical directions")
            }
        }
    }

    ScrollBar.vertical: ScrollBar {
        policy: alwaysOnCheckBox.checked ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
    }
}
