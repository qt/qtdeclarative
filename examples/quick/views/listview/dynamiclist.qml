// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import "content"

// This example shows how items can be dynamically added to and removed from
// a ListModel, and how these list modifications can be animated.

pragma ComponentBehavior: Bound

Rectangle {
    id: container
    width: 500
    height: 400
    color: "#343434"

    // The model:
    ListModel {
        id: fruitModel

        ListElement {
            name: "Apple"
            cost: 2.45
            attributes: [
                ListElement {
                    description: "Core"
                },
                ListElement {
                    description: "Deciduous"
                }
            ]
        }
        ListElement {
            name: "Banana"
            cost: 1.95
            attributes: [
                ListElement {
                    description: "Tropical"
                },
                ListElement {
                    description: "Seedless"
                }
            ]
        }
        ListElement {
            name: "Cumquat"
            cost: 3.25
            attributes: [
                ListElement {
                    description: "Citrus"
                }
            ]
        }
        ListElement {
            name: "Durian"
            cost: 9.95
            attributes: [
                ListElement {
                    description: "Tropical"
                },
                ListElement {
                    description: "Smelly"
                }
            ]
        }
    }

    // The delegate for each fruit in the model:
    Component {
        id: listDelegate
//! [0]
        Item {
//! [0]
            id: delegateItem
            width: listView.width
            height: 80
            clip: true

            required property int index
            required property string name
            required property real cost
            required property var attributes

            Column {
                id: arrows
                anchors {
                    left: parent.left
                    verticalCenter: parent.verticalCenter
                }
                Image {
                    source: "content/pics/arrow-up.png"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: fruitModel.move(delegateItem.index, delegateItem.index - 1, 1)
                    }
                }
                Image { source: "content/pics/arrow-down.png"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: fruitModel.move(delegateItem.index, delegateItem.index + 1, 1)
                    }
                }
            }

            Column {
                anchors {
                    left: arrows.right
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.verticalCenter
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: delegateItem.name
                    font.pixelSize: 15
                    color: "white"
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 5
                    Repeater {
                        model: delegateItem.attributes
                        Text {
                            required property string description
                            text: description
                            color: "White"
                        }
                    }
                }
            }

            Item {
                anchors {
                    left: arrows.right
                    horizontalCenter: parent.horizontalCenter;
                    top: parent.verticalCenter
                    bottom: parent.bottom
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 10

                    PressAndHoldButton {
                        anchors.verticalCenter: parent.verticalCenter
                        source: "content/pics/plus-sign.png"
                        onClicked: fruitModel.setProperty(delegateItem.index, "cost", delegateItem.cost + 0.25)
                    }

                    Text {
                        id: costText
                        anchors.verticalCenter: parent.verticalCenter
                        text: '$' + Number(delegateItem.cost).toFixed(2)
                        font.pixelSize: 15
                        color: "white"
                        font.bold: true
                    }

                    PressAndHoldButton {
                        anchors.verticalCenter: parent.verticalCenter
                        source: "content/pics/minus-sign.png"
                        onClicked: fruitModel.setProperty(delegateItem.index, "cost",
                                                          Math.max(0, delegateItem.cost - 0.25))
                    }

                    Image {
                        source: "content/pics/list-delete.png"
                        MouseArea {
                            anchors.fill: parent
                            onClicked: fruitModel.remove(delegateItem.index)
                        }
                    }
                }
            }

            // Animate adding and removing of items:
//! [1]
            SequentialAnimation {
                id: addAnimation
                PropertyAction {
                    target: delegateItem
                    property: "height"
                    value: 0
                }
                NumberAnimation {
                    target: delegateItem
                    property: "height"
                    to: 80
                    duration: 250
                    easing.type: Easing.InOutQuad
                }
            }
            ListView.onAdd: addAnimation.start()

            SequentialAnimation {
                id: removeAnimation
                PropertyAction {
                    target: delegateItem
                    property: "ListView.delayRemove"
                    value: true
                }
                NumberAnimation {
                    target: delegateItem
                    property: "height"
                    to: 0
                    duration: 250
                    easing.type: Easing.InOutQuad
                }

                // Make sure delayRemove is set back to false so that the item can be destroyed
                PropertyAction {
                    target: delegateItem
                    property: "ListView.delayRemove"
                    value: false
                }
            }
            ListView.onRemove: removeAnimation.start()
        }
//! [1]
    }

    // The view:
    ListView {
        id: listView
        anchors {
            left: parent.left
            top: parent.top
            right: parent.right
            bottom: buttons.top
            margins: 20
        }
        model: fruitModel
        delegate: listDelegate
    }

    Row {
        id: buttons
        anchors {
            left: parent.left
            bottom: parent.bottom
            margins: 20
        }
        spacing: 10

        TextButton {
            text: qsTr("Add an item")
            onClicked: {
                fruitModel.append({
                    "name": "Pizza Margarita",
                    "cost": 5.95,
                    "attributes": [{"description": "Cheese"}, {"description": "Tomato"}]
                })
            }
        }

        TextButton {
            text: qsTr("Remove all items")
            onClicked: fruitModel.clear()
        }
    }
}

