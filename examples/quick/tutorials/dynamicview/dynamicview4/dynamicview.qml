// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models

Rectangle {
    id: root

    width: 300
    height: 400

    Component {
        id: dragDelegate

        MouseArea {
            id: dragArea

            property bool held: false
            required property string name
            required property string type
            required property string size
            required property int age

            anchors {
                left: parent?.left
                right: parent?.right
            }
            height: content.height

            enabled: visualModel.sortOrder == visualModel.lessThan.length

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressAndHold: held = true
            onReleased: held = false

            Rectangle {
                id: content

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: dragArea.width
                height: column.implicitHeight + 4

                border.width: 1
                border.color: "lightsteelblue"

                color: dragArea.held ? "lightsteelblue" : "white"
                Behavior on color { ColorAnimation { duration: 100 } }

                radius: 2

                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                states: State {
                    when: dragArea.held

                    ParentChange {
                        target: content
                        parent: root
                    }
                    AnchorChanges {
                        target: content
                        anchors {
                            horizontalCenter: undefined
                            verticalCenter: undefined
                        }
                    }
                }

                Column {
                    id: column
                    anchors {
                        fill: parent
                        margins: 2
                    }

                    Text { text: qsTr('Name: ') + dragArea.name }
                    Text { text: qsTr('Type: ') + dragArea.type }
                    Text { text: qsTr('Age: ') + dragArea.age }
                    Text { text: qsTr('Size: ') + dragArea.size }
                }
            }

            DropArea {
                anchors {
                    fill: parent
                    margins: 10
                }

                onEntered: (drag) => {
                    visualModel.items.move(
                            drag.source.DelegateModel.itemsIndex,
                            dragArea.DelegateModel.itemsIndex)
                }
            }
        }
    }
//![0]
    DelegateModel {
        id: visualModel
//![4]
        property var lessThan: [
            function(left, right) { return left.name < right.name },
            function(left, right) { return left.type < right.type },
            function(left, right) { return left.age < right.age },
            function(left, right) {
                if (left.size == "Small")
                    return true
                else if (right.size == "Small")
                    return false
                else if (left.size == "Medium")
                    return true
                else
                    return false
            }
        ]
//![4]
//![6]

        property int sortOrder: orderSelector.selectedIndex
        onSortOrderChanged: items.setGroups(0, items.count, "unsorted")

//![6]
//![3]
        function insertPosition(lessThan, item) {
            let lower = 0
            let upper = items.count
            while (lower < upper) {
                const middle = Math.floor(lower + (upper - lower) / 2)
                const result = lessThan(item.model, items.get(middle).model)
                if (result) {
                    upper = middle
                } else {
                    lower = middle + 1
                }
            }
            return lower
        }

        function sort(lessThan) {
            while (unsortedItems.count > 0) {
                const item = unsortedItems.get(0)
                const index = insertPosition(lessThan, item)

                item.groups = "items"
                items.move(item.itemsIndex, index)
            }
        }
//![3]

//![1]
        items.includeByDefault: false
//![5]
        groups: DelegateModelGroup {
            id: unsortedItems
            name: "unsorted"

            includeByDefault: true
//![1]
            onChanged: {
                if (visualModel.sortOrder == visualModel.lessThan.length)
                    setGroups(0, count, "items")
                else
                    visualModel.sort(visualModel.lessThan[visualModel.sortOrder])
            }
//![2]
        }
//![2]
//![5]
        model: PetsModel {}
        delegate: dragDelegate
    }
//![0]
    ListView {
        id: view

        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            bottom: orderSelector.top
            margins: 2
        }

        model: visualModel

        spacing: 4
        cacheBuffer: 50
    }

    ListSelector {
        id: orderSelector

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 2
        }

        label: qsTr("Sort By")
        list: [ qsTr("Name"), qsTr("Type"), qsTr("Age"), qsTr("Size"), qsTr("Custom") ]
    }
}
