// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.14
import QtQml.Models 2.14

Item {
    id: root
    width: 400
    height: 400

    readonly property int rectCount: 3
    property var rectColors: ["red", "green", "blue"]

    property alias listView1: listView1
    property alias listView2: listView2

    function moveRedRectToModel2() {
        var appItem = objectModel1.get(0)
        objectModel1.remove(0, 1)
        objectModel2.insert(0, appItem)
    }

    function moveRedRectToModel1() {
        var appItem = objectModel2.get(0)
        objectModel2.remove(0, 1)
        objectModel1.insert(0, appItem)
    }

    ObjectModel {
        id: objectModel1
        objectName: "objectModel1"

        Component.onCompleted: {
            for (var i = 0; i < root.rectCount; i++) {
                var outerRect = rectComponent.createObject(null, {
                    "objectName": root.rectColors[i] + "Rect",
                    "color": root.rectColors[i]
                })
                objectModel1.append(outerRect)
            }
        }
    }

    ObjectModel {
        id: objectModel2
        objectName: "objectModel2"
    }

    ListView {
        id: listView1
        objectName: "listView1"
        anchors.left: parent.left
        anchors.top: parent.top
        height: 100
        width: 100
        anchors.margins: 20
        clip: true
        cacheBuffer: 0
        model: objectModel1
        orientation: ListView.Horizontal
        spacing: 20

        Component.onCompleted: contentItem.objectName = "listView1ContentItem"
    }

    ListView {
        id: listView2
        objectName: "listView2"
        anchors.right: parent.right
        anchors.top: parent.top
        height: 100
        width: 100
        anchors.margins: 20
        clip: true
        cacheBuffer: 0
        model: objectModel2
        orientation: ListView.Horizontal
        spacing: 20

        Component.onCompleted: contentItem.objectName = "listView2ContentItem"
    }

    Component {
        id: rectComponent

        Rectangle {
            height: 100
            width: 100
            opacity: 0.2
        }
    }
}
