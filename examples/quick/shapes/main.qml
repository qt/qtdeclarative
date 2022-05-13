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
            addExample("Shape Gallery", "Simple path rendering examples", Qt.resolvedUrl("shapegallery.qml"))
            addExample("Interactive Shape", "Dynamic, interactive path rendering examples", Qt.resolvedUrl("interactive.qml"))
            addExample("Super- and multisampling", "Improving quality", Qt.resolvedUrl("sampling.qml"))
            addExample("Clip My Tiger!", "Clip examples, a.k.a. What Not To Do", Qt.resolvedUrl("clippedtigers.qml"))
        }
    }
}
