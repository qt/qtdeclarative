// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQml.Models

Item {
    width: 500
    height: 200

    property list<QtObject> myModel: [
        QtObject {
            objectName: "Item 0"
            property bool selected: true
        },
        QtObject {
            objectName: "Item 1"
            property bool selected: false
        },
        QtObject {
            objectName: "Item 2"
            property bool selected: false
        },
        QtObject {
            objectName: "Item 3"
            property bool selected: true
        },
        QtObject {
            objectName: "Item 4"
            property bool selected: true
        },
        QtObject {
            objectName: "Item 5"
            property bool selected: true
        },
        QtObject {
            objectName: "Item 6"
            property bool selected: false
        }
    ]

    ListView {
        objectName: "listView"
        id: listview
        width: 500
        height: 200

        focus: true
        clip: true
        spacing: 2
        orientation: ListView.Horizontal
        highlightMoveDuration: 300
        highlightMoveVelocity: -1
        preferredHighlightBegin: (500 - 100) / 2
        preferredHighlightEnd: (500 + 100) / 2
        highlightRangeMode: ListView.StrictlyEnforceRange
        cacheBuffer: 500
        currentIndex: 1

        model: DelegateModel {
            id: delegateModel
            filterOnGroup: "visible"
            model: myModel
            groups: [
                DelegateModelGroup {
                    name: "visible"
                    includeByDefault: true
                }
            ]
            delegate: Rectangle {
                id: tile
                objectName: model.modelData.objectName

                width: 100
                height: 100
                border.width: 0
                anchors.verticalCenter: parent.verticalCenter

                visible: model.modelData.selected
                Component.onCompleted: {
                    DelegateModel.inPersistedItems = true
                    DelegateModel.inVisible = Qt.binding(function () {
                        return model.modelData.selected
                    })
                }

                property bool isCurrent: ListView.isCurrentItem
                color: isCurrent ? "red" : "green"

                Text {
                    id: valueText
                    anchors.centerIn: parent
                    text: model.modelData.objectName
                }
            }
        }
    }
}
