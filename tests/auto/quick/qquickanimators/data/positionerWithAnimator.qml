// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2

Column {
    width: 200
    height: 200

    property alias repeater: repeater
    property alias transition: transition

    anchors.centerIn: parent
    populate: Transition {
        id: transition
        ScaleAnimator {
            from: 0
            to: 1
        }
    }

    Repeater {
        id: repeater
        model: ["red", "green", "blue"]

        Rectangle {
            width: 100
            height: 100
            color: modelData
            scale: 0
        }
    }
}
