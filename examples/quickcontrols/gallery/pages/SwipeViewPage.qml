// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Pane {
    id: pane

    SwipeView {
        id: view
        currentIndex: 1
        anchors.fill: parent

        Repeater {
            model: 3

            Pane {
                width: SwipeView.view.width
                height: SwipeView.view.height

                Column {
                    spacing: 40
                    width: parent.width

                    Label {
                        width: parent.width
                        wrapMode: Label.Wrap
                        horizontalAlignment: Qt.AlignHCenter
                        text: "SwipeView provides a navigation model that simplifies horizontal paged scrolling. "
                        + "The page indicator on the bottom shows which is the presently active page."
                    }

                    Image {
                        source: "../images/arrows.png"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }
    }

    PageIndicator {
        count: view.count
        currentIndex: view.currentIndex
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
