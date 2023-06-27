// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
import QtQuick

//! [mywidget]
Rectangle {
    id: widget
    color: "lightsteelblue"; width: 175; height: 25; radius: 10; antialiasing: true
    Text { id: label; anchors.centerIn: parent}
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
//! [mywidget]
