// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.15
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import Qt.labs.qmlmodels 1.0
import TestModel 0.1

Window {
    id: root
    visible: true
    width: 640
    height: 480

    property int rows: 500
    property int columns: 20
    property real delegateHeight: 10
    property real delegateWidth: 50

    CheckBox {
        id: reuseItemsBox
        text: "Reuse items"
        checked: true
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        anchors.topMargin: reuseItemsBox.height + 10
        color: "lightgray"

        ListView {
            id: listView
            anchors.fill: parent

            reuseItems: reuseItemsBox.checked

            cacheBuffer: 0
            contentWidth: columns * delegateWidth
            contentHeight: rows * delegateHeight
            flickableDirection: Flickable.HorizontalAndVerticalFlick
            clip: true

            model: TestModel {
                rowCount: root.rows
            }

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOn }

            delegate: DelegateChooser {
                role: "delegateType"

                DelegateChoice {
                    roleValue: "type1"

                    Item {
                        width: listView.contentWidth
                        height: delegateHeight
                        property int reusedCount: 0

                        ListView.onReused: reusedCount++

                        Text {
                            id: text1
                            text: "Reused count:" + reusedCount
                            font.pixelSize: 9
                        }

                        Row {
                            id: choice1
                            width: listView.contentWidth
                            height: delegateHeight
                            anchors.left: text1.right
                            anchors.leftMargin: 5
                            property color color: Qt.rgba(0.6, 0.6, 0.8, 1)

                            Component.onCompleted: {
                                for (var i = 0; i < columns; ++i)
                                    cellComponent.createObject(choice1, {column: i, color: color})
                            }
                        }
                    }
                }

                DelegateChoice {
                    roleValue: "type2"

                    Item {
                        width: listView.contentWidth
                        height: delegateHeight
                        property int reusedCount: 0

                        ListView.onReused: reusedCount++

                        Text {
                            id: text2
                            text: "Reused count:" + reusedCount
                            font.pixelSize: 9
                        }

                        Row {
                            id: choice2
                            width: listView.contentWidth
                            height: delegateHeight
                            anchors.left: text2.right
                            anchors.leftMargin: 5
                            property color color: Qt.rgba(0.3, 0.3, 0.8, 1)

                            Component.onCompleted: {
                                for (var i = 0; i < columns; ++i)
                                    cellComponent.createObject(choice2, {column: i, color: color})
                            }
                        }
                    }
                }
            }

        }
    }

    Component {
        id: cellComponent
        Rectangle {
            height: delegateHeight
            width: delegateWidth
            property int column
            Text {
                text: "Lorem ipsum dolor sit amet"
                font.pixelSize: 9
            }
        }
    }
}
