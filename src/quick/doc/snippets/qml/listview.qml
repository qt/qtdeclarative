// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [document]
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]
    width: 175; height: 175; color: "white"

//! [model]
ListModel {
    id: petlist
    ListElement { type: "Cat" }
    ListElement { type: "Dog" }
    ListElement { type: "Mouse" }
    ListElement { type: "Rabbit" }
    ListElement { type: "Horse" }
}
//! [model]

//! [delegate]
Component {
    id: petdelegate
    Text {
        id: label
        font.pixelSize: 24
        text: index === 0 ? type + " (default)" : type

        required property int index
        required property string type
    }
}
//! [delegate]

//! [view]
ListView {
    id: view
    anchors.fill: parent

    model: petlist
    delegate: petdelegate
}
//! [view]

//! [parent end]
}
//! [parent end]
//! [document]
