// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root

    property alias source: source
    property alias primaryTarget: primaryTarget
    property alias secondaryTarget: secondaryTarget

    property var pressedKeys: []
    property var releasedKeys: []
    Keys.onPressed: { var keys = pressedKeys; keys.push(event.key); pressedKeys = keys }
    Keys.onReleased: { var keys = releasedKeys; keys.push(event.key); releasedKeys = keys }

    Item {
        id: primaryTarget
        objectName: "primary"
        property var pressedKeys: []
        property var releasedKeys: []
        Keys.forwardTo: [ secondaryTarget, extraTarget ]
        Keys.onPressed: { event.accepted = event.key === Qt.Key_P; var keys = pressedKeys; keys.push(event.key); pressedKeys = keys }
        Keys.onReleased: { event.accepted = event.key === Qt.Key_P; var keys = releasedKeys; keys.push(event.key); releasedKeys = keys }

        Item {
            id: source
            objectName: "source"
            property var pressedKeys: []
            property var releasedKeys: []
            Keys.forwardTo: primaryTarget
            Keys.onPressed: { var keys = pressedKeys; keys.push(event.key); pressedKeys = keys }
            Keys.onReleased: { var keys = releasedKeys; keys.push(event.key); releasedKeys = keys }
        }
    }

    Item {
        id: secondaryTarget
        objectName: "secondary"
        property var pressedKeys: []
        property var releasedKeys: []
        Keys.onPressed: { event.accepted = event.key === Qt.Key_S; var keys = pressedKeys; keys.push(event.key); pressedKeys = keys }
        Keys.onReleased: { event.accepted = event.key === Qt.Key_S; var keys = releasedKeys; keys.push(event.key); releasedKeys = keys }
    }

    Item {
        id: extraTarget
    }
}
