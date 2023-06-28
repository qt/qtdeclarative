// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQml
import QtQuick

pragma ComponentBehavior: Bound

Rectangle {
    id: container

    property string text: qsTr("Drag me!")
    property bool animated: true

    width: 320; height: 480; color: "#474747"; focus: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
            container.remove()
        else if (event.text !== "") {
            container.append(event.text)
        }
    }

    function append(text) {
        container.animated = false
        const lastLetter = container.children[container.children.length - 1]
        let newLetter = letterComponent.createObject(container)
        newLetter.text = text
        newLetter.follow = lastLetter
        container.animated = true
    }

    function remove() {
        if (container.children.length)
            container.children[container.children.length - 1].destroy()
    }

    function doLayout() {
        var follow = null
        for (let i = 0; i < container.text.length; ++i) {
            let newLetter = letterComponent.createObject(container)
            newLetter.text = container.text[i]
            newLetter.follow = follow
            follow = newLetter
        }
    }

    Component {
        id: letterComponent
        Text {
            id: letter
            property variant follow

//! [0]
            x: follow ? follow.x + follow.width : container.width / 6
            y: follow ? follow.y : container.height / 2
//! [0]

            font.pixelSize: 40; font.bold: true
            color: "#999999"; styleColor: "#222222"; style: Text.Raised

            MouseArea {
                anchors.fill: parent
                drag.target: letter; drag.axis: Drag.XAndYAxis
                onPressed: letter.color = "#dddddd"
                onReleased: letter.color = "#999999"
            }

//! [1]
            Behavior on x { enabled: container.animated; SpringAnimation { spring: 3; damping: 0.3; mass: 1.0 } }
            Behavior on y { enabled: container.animated; SpringAnimation { spring: 3; damping: 0.3; mass: 1.0 } }
//! [1]
        }
    }

    Component.onCompleted: doLayout()
}
