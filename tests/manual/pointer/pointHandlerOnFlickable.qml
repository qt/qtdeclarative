// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12
import "content"

Flickable {
    id: root
    objectName: "root"
    width: 800
    height: 480
    contentWidth: 1000
    contentHeight: 600

    Rectangle {
        objectName: "button"
        anchors.centerIn: parent
        border.color: "tomato"
        border.width: 10
        color: innerTap.pressed ? "wheat" : "transparent"
        width: 100
        height: 100
        TapHandler {
            id: innerTap
            objectName: "buttonTap"
        }
    }

    TapHandler {
        id: contentItemTap
        objectName: "contentItemTap"
        onLongPressed: longPressFeedback.createObject(root.contentItem,
            {"x": point.position.x, "y": point.position.y,
             "text": contentItemTap.timeHeld.toFixed(3) + " sec"})

    }
    TouchpointFeedbackSprite { }
    TouchpointFeedbackSprite { }
    TouchpointFeedbackSprite { }
    TouchpointFeedbackSprite { }
    TouchpointFeedbackSprite { }

    Component {
        id: longPressFeedback
        Text { }
    }

    Component.onCompleted: contentItem.objectName = "Flickable's contentItem"
}
