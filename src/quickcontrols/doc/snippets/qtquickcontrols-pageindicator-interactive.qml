// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls

Pane {
//! [1]
SwipeView {
    id: view
    currentIndex: pageIndicator.currentIndex
    anchors.fill: parent

    Page {
        title: qsTr("Home")
    }
    Page {
        title: qsTr("Discover")
    }
    Page {
        title: qsTr("Activity")
    }
}

PageIndicator {
    id: pageIndicator
    interactive: true
    count: view.count
    currentIndex: view.currentIndex

    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter
}
//! [1]
}
