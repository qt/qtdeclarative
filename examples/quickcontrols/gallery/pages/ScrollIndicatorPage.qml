// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Flickable {
    id: flickable

    contentHeight: pane.height

    Pane {
        id: pane
        width: flickable.width
        height: flickable.height * 1.25

        Column {
            id: column
            spacing: 40
            width: parent.width

            Label {
                width: parent.width
                wrapMode: Label.Wrap
                horizontalAlignment: Qt.AlignHCenter
                text: "ScrollIndicator is a non-interactive indicator that indicates the current scroll position. "
                    + "A scroll indicator can be either vertical or horizontal, and can be attached to any Flickable, "
                    + "such as ListView and GridView."
            }

            Image {
                rotation: 90
                source: "../images/arrows.png"
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    ScrollIndicator.vertical: ScrollIndicator { }
}
