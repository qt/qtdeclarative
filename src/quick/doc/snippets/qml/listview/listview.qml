// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![import]
import QtQuick
//![import]

Item {

//![classdocs simple]
ListView {
    width: 180; height: 200

    model: ContactModel {}
    delegate: Text {
        required property string name
        required property string number
        text: name + ": " + number
    }
}
//![classdocs simple]

//![classdocs advanced]
Rectangle {
    width: 180; height: 200

    Component {
        id: contactDelegate
        Item {
            id: myItem
            required property string name
            required property string number
            width: 180; height: 40
            Column {
                Text { text: '<b>Name:</b> ' + myItem.name }
                Text { text: '<b>Number:</b> ' + myItem.number }
            }
        }
    }

    ListView {
        anchors.fill: parent
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
        SequentialAnimation {
            id: removeAnimation
            PropertyAction { target: wrapper; property: "ListView.delayRemove"; value: true }
            NumberAnimation { target: wrapper; property: "scale"; to: 0; duration: 250; easing.type: Easing.InOutQuad }
            PropertyAction { target: wrapper; property: "ListView.delayRemove"; value: false }
        }
        ListView.onRemove: removeAnimation.start()
    }
}
//![delayRemove]

//![highlightFollowsCurrentItem]
Component {
    id: highlight
    Rectangle {
        width: 180; height: 40
        color: "lightsteelblue"; radius: 5
        y: list.currentItem.y
        Behavior on y {
            SpringAnimation {
                spring: 3
                damping: 0.2
            }
        }
    }
}

ListView {
    id: list
    width: 180; height: 200
    model: ContactModel {}
    delegate: Text { text: name }

    highlight: highlight
    highlightFollowsCurrentItem: false
    focus: true
}
//![highlightFollowsCurrentItem]

//![isCurrentItem]
ListView {
    width: 180; height: 200

    Component {
        id: contactsDelegate
        Rectangle {
            id: wrapper
            width: 180
            height: contactInfo.height
            color: ListView.isCurrentItem ? "black" : "red"
            Text {
                id: contactInfo
                text: name + ": " + number
                color: wrapper.ListView.isCurrentItem ? "red" : "black"
            }
        }
    }

    model: ContactModel {}
    delegate: contactsDelegate
    focus: true
}
//![isCurrentItem]

//![flickBothDirections]
ListView {
    width: 180; height: 200

    contentWidth: 320
    flickableDirection: Flickable.AutoFlickDirection

    model: ContactModel {}
    delegate: Row {
        Text { text: '<b>Name:</b> ' + name; width: 160 }
        Text { text: '<b>Number:</b> ' + number; width: 160 }
    }
}
//![flickBothDirections]

}
