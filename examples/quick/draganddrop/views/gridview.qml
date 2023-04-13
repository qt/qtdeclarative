// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound
import QtQml
import QtQuick
import QtQml.Models

GridView {
    id: root
    width: 320
    height: 480
    cellWidth: 80
    cellHeight: 80

    displaced: Transition {
        NumberAnimation {
            properties: "x,y"
            easing.type: Easing.OutQuad
        }
    }

//! [0]
    model: DelegateModel {
//! [0]
        id: visualModel
        model: ListModel {
            id: colorModel
            ListElement { color: "blue" }
            ListElement { color: "green" }
            ListElement { color: "red" }
            ListElement { color: "yellow" }
            ListElement { color: "orange" }
            ListElement { color: "purple" }
            ListElement { color: "cyan" }
            ListElement { color: "magenta" }
            ListElement { color: "chartreuse" }
            ListElement { color: "aquamarine" }
            ListElement { color: "indigo" }
            ListElement { color: "black" }
            ListElement { color: "lightsteelblue" }
            ListElement { color: "violet" }
            ListElement { color: "grey" }
            ListElement { color: "springgreen" }
            ListElement { color: "salmon" }
            ListElement { color: "blanchedalmond" }
            ListElement { color: "forestgreen" }
            ListElement { color: "pink" }
            ListElement { color: "navy" }
            ListElement { color: "goldenrod" }
            ListElement { color: "crimson" }
            ListElement { color: "teal" }
        }
//! [1]
        delegate: DropArea {
            id: delegateRoot
            required property color color

            width: 80
            height: 80

            onEntered: function(drag) {
                visualModel.items.move((drag.source as Icon).visualIndex, icon.visualIndex)
            }

            property int visualIndex: DelegateModel.itemsIndex

            Icon {
                id: icon
                dragParent: root
                visualIndex: delegateRoot.visualIndex
                color: delegateRoot.color
            }
        }
//! [1]
    }
}
