// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import shared

Item {
    height: 480
    width: 320
    LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("Multipoint Flames", "Create multiple flames with multiple fingers", Qt.resolvedUrl("multipointtouch/multiflame.qml"));
            addExample("Bear-Whack", "Use multiple touches to knock all the bears down",  Qt.resolvedUrl("multipointtouch/bearwhack.qml"));
            addExample("Flick Resize", "Manipulate images using pinch gestures", Qt.resolvedUrl("pincharea/flickresize.qml"));
            addExample("Flickable", "A viewport you can move with touch gestures", Qt.resolvedUrl("flickable/basic-flickable.qml"));
            addExample("Corkboards", "Uses touch input on items inside a Flickable", Qt.resolvedUrl("flickable/corkboards.qml"));
        }
    }
}
