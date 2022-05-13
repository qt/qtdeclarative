// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    width: 800
    height: 800
    Component.onCompleted: { list.model.remove(0); }
    ListView {
        id: list
        objectName: "list"
        anchors.fill: parent
        model: ListModel {
            ListElement {
                txt: "Foo"
            }
        }
        delegate: Rectangle {
            id: myDelegate
            color: "red"
            width: 800
            height: 100
            ListView.onRemove: SequentialAnimation {
               PropertyAction { target: myDelegate; property: "ListView.delayRemove"; value: true }
               NumberAnimation { target: myDelegate; property: "scale"; to: 0; duration: 1; }
               PropertyAction { target: myDelegate; property: "ListView.delayRemove"; value: false }
            }

        }
        footer: Rectangle {
            id: listFooter
            color: "blue"
            width: 800
            height: 100
        }
    }
}
