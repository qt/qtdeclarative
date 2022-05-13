// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    id: flashingblob
    width: 300; height: 300

    MouseArea {
        anchors.fill: parent
        onClicked: {
            animatePosition.start()
        }
    }

    Rectangle {
        id : animatee
        width: 100; height: 100
        y : 100
        color: "blue"
        opacity: 0.5
        Text {
            anchors.centerIn: parent
            text : "Be Well"
        }
    }

    NumberAnimation {
        id: animatePosition
        target: animatee
        properties: "x"
        from: animatee.x
        to: animatee.x + 50
        loops: 1
        easing {type: Easing.Linear;}
    }
}
