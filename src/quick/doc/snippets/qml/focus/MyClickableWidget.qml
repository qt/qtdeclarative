// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [clickable in focusscope]
FocusScope {

    id: scope

    //FocusScope needs to bind to visual properties of the children
    property alias color: rectangle.color
    x: rectangle.x; y: rectangle.y
    width: rectangle.width; height: rectangle.height

    Rectangle {
        id: rectangle
        anchors.centerIn: parent
        color: "lightsteelblue"; width: 175; height: 25; radius: 10; antialiasing: true
        Text { id: label; anchors.centerIn: parent }
        focus: true
        Keys.onPressed: (event)=> {
            if (event.key == Qt.Key_A)
                label.text = 'Key A was pressed'
            else if (event.key == Qt.Key_B)
                label.text = 'Key B was pressed'
            else if (event.key == Qt.Key_C)
                label.text = 'Key C was pressed'
        }
    }
    MouseArea { anchors.fill: parent; onClicked: { scope.focus = true } }
}
//! [clickable in focusscope]
