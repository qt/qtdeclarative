// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        // TODO: no down property to test this with
//        ["pressed"]
    ]

    property Component component: PageIndicator {
        enabled: !is("disabled")
        count: 5
    }

    property Component exampleComponent: ColumnLayout {
        implicitWidth: 200
        implicitHeight: 200

        // TODO: why doesn't this fill the ColumnLayout? :/
        StackLayout {
            id: swipeView
            currentIndex: pageIndicator.currentIndex

            Label {
                text: qsTr("Page 1")
                horizontalAlignment: Label.AlignHCenter
            }

            Label {
                text: qsTr("Page 2")
                horizontalAlignment: Label.AlignHCenter
            }

            Label {
                text: qsTr("Page 3")
                horizontalAlignment: Label.AlignHCenter
            }

            Label {
                text: qsTr("Page 4")
                horizontalAlignment: Label.AlignHCenter
            }

            Label {
                text: qsTr("Page 5")
                horizontalAlignment: Label.AlignHCenter
            }
        }

        PageIndicator {
            id: pageIndicator
            currentIndex: swipeView.currentIndex
            count: swipeView.count
            interactive: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
