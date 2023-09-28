// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

Item {
    width: 1280
    height: 720
    LauncherList {
        anchors.fill: parent
        Component.onCompleted: {
            addExample(qsTr("Shape Gallery"), qsTr("Simple path rendering examples"), Qt.resolvedUrl("shapegallery.qml"))
            addExample(qsTr("Interactive Shape"), qsTr("Dynamic, interactive path rendering examples"), Qt.resolvedUrl("interactive.qml"))
            addExample(qsTr("Anti-aliasing"), qsTr("Improving quality"), Qt.resolvedUrl("sampling.qml"))
            addExample(qsTr("Magnify My Tiger!"), qsTr("Path zooming example"), Qt.resolvedUrl("zoomtiger.qml"))
            addExample(qsTr("Clip My Tiger!"), qsTr("Clip examples, a.k.a. What Not To Do"), Qt.resolvedUrl("clippedtigers.qml"))
        }
    }
}
