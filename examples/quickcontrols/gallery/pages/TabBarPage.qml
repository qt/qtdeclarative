// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Page {
    id: page

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

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
                        text: "TabBar is a bar with icons or text which allows the user "
                              + "to switch between different subtasks, views, or modes."
                    }

                    Image {
                        source: "../images/arrows.png"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }
    }

    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex

        TabButton {
            text: "First"
        }
        TabButton {
            text: "Second"
        }
        TabButton {
            text: "Third"
        }
    }
}
