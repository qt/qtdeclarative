// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick

//! [FocusScope delegate]
Rectangle {
    color: "lightsteelblue"; width: 100; height: 50

    ListView {
        anchors.fill: parent
        focus: true

        model: ListModel {
            ListElement { name: "Bob" }
            ListElement { name: "John" }
            ListElement { name: "Michael" }
        }

        delegate: FocusScope {
                width: childrenRect.width; height: childrenRect.height
                x:childrenRect.x; y: childrenRect.y
                TextInput {
                    focus: true
                    text: name
                    Keys.onReturnPressed: console.log(name)
                }
        }
    }
}
//! [FocusScope delegate]
