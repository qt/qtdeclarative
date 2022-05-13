// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared as Examples

Item {
    height: 480
    width: 320
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Layout Direction", "Shows RTL layouting in positioners and views",  Qt.resolvedUrl("layoutdirection/layoutdirection.qml"));
            addExample("Layout Mirroring", "Shows RTL layouting in basic text and anchors", Qt.resolvedUrl("layoutmirroring/layoutmirroring.qml"));
            addExample("Text Alignment", "Shows RTL layouting in various text elements", Qt.resolvedUrl("textalignment/textalignment.qml"));
        }
    }
}
