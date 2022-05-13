// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 300
    height: 300
    visible: true

    property alias swipeView: view

    Rectangle {
        border.width: 1
        anchors.fill: parent
    }

    SwipeView {
        id: view
        anchors.fill: parent

        Label {
            text: "First\nPage"
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }

        Label {
            text: "Second\nPage"
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }

        Label {
            text: "Third\nPage"
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }
    }

    PageIndicator {
        id: indicator

        count: view.count
        currentIndex: view.currentIndex

        anchors.bottom: view.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
