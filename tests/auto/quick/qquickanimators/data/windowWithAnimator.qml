// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2
import QtQuick.Window 2.0

Window {
    width: 200
    height: 200

    visible: true
    property bool animationDone: rect.scale == 1;

    Rectangle {
        id: rect
        anchors.centerIn: parent

        width: 100
        height: 100
        color: "red"
        scale: 0

        ScaleAnimator on scale {
            id: animation;
            from: 0
            to: 1
            duration: 1000
        }
    }
}
