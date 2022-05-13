// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick 2.15

Rectangle {
    id: root
    width: 640
    height: 480

    property int rows: 500
    property int columns: 20
    property real delegateHeight: 30
    property real delegateWidth: 50

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 10
        objectName: "list"

        model: reuseModel
        reuseItems: true

        cacheBuffer: 0
        contentWidth: columns * delegateWidth
        contentHeight: rows * delegateHeight
        clip: true

        property int delegatesCreatedCount: 0

        delegate: Item {
            objectName: "delegate"
            width: list.contentWidth
            height: delegateHeight

            property int modelIndex: index
            property int reusedCount: 0
            property int pooledCount: 0
            property string displayBinding: display

            ListView.onPooled: pooledCount++
            ListView.onReused: reusedCount++
            Component.onCompleted: list.delegatesCreatedCount++

            Text {
                id: text1
                text: display + " (Model index: " + modelIndex + ", Reused count: " + reusedCount + ")"
            }
        }
    }
}
