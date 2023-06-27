// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![import]
import QtQuick
//![import]

Rectangle {
    width: childrenRect.width; height: childrenRect.height

Row {

//![classdocs simple]
GridView {
    width: 300; height: 200

    model: ContactModel {}
    delegate: Column {
        Image { source: portrait; anchors.horizontalCenter: parent.horizontalCenter }
        Text { text: name; anchors.horizontalCenter: parent.horizontalCenter }
    }
}
//![classdocs simple]


//![classdocs advanced]
Rectangle {
    width: 300; height: 200

    Component {
        id: contactDelegate
        Item {
            width: grid.cellWidth; height: grid.cellHeight
            Column {
                anchors.fill: parent
                Image { source: portrait; anchors.horizontalCenter: parent.horizontalCenter }
                Text { text: name; anchors.horizontalCenter: parent.horizontalCenter }
            }
        }
    }

    GridView {
        id: grid
        anchors.fill: parent
        cellWidth: 80; cellHeight: 80

        model: ContactModel {}
        delegate: contactDelegate
        highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
        focus: true
    }
}
//![classdocs advanced]

//![delayRemove]
Component {
    id: delegate
    Item {
        GridView.onRemove: SequentialAnimation {
            PropertyAction { target: wrapper; property: "GridView.delayRemove"; value: true }
            NumberAnimation { target: wrapper; property: "scale"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
            PropertyAction { target: wrapper; property: "GridView.delayRemove"; value: false }
        }
    }
}
//![delayRemove]

//![highlightFollowsCurrentItem]
Component {
    id: highlight
    Rectangle {
        width: view.cellWidth; height: view.cellHeight
        color: "lightsteelblue"; radius: 5
        x: view.currentItem.x
        y: view.currentItem.y
        Behavior on x { SpringAnimation { spring: 3; damping: 0.2 } }
        Behavior on y { SpringAnimation { spring: 3; damping: 0.2 } }
    }
}

GridView {
    id: view
    width: 300; height: 200
    cellWidth: 80; cellHeight: 80

    model: ContactModel {}
    delegate: Column {
        Image { source: portrait; anchors.horizontalCenter: parent.horizontalCenter }
        Text { text: name; anchors.horizontalCenter: parent.horizontalCenter }
    }

    highlight: highlight
    highlightFollowsCurrentItem: false
    focus: true
}
//![highlightFollowsCurrentItem]

//![isCurrentItem]
GridView {
    width: 300; height: 200
    cellWidth: 80; cellHeight: 80

    Component {
        id: contactsDelegate
        Rectangle {
            id: wrapper
            width: 80
            height: 80
            color: GridView.isCurrentItem ? "black" : "red"
            Text {
                id: contactInfo
                text: name + ": " + number
                color: wrapper.GridView.isCurrentItem ? "red" : "black"
            }
        }
    }

    model: ContactModel {}
    delegate: contactsDelegate
    focus: true
}
//![isCurrentItem]

}

}
