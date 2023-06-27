// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
import QtQuick

//! [parent begin]
Rectangle {
//! [parent begin]
    id: screen
    width: 175; height: 175
    color: "lightgrey"

//! [define inline component]
    Component {
        id: inlinecomponent
        Rectangle {
            id: display
            width: 50; height: 50
            color: "blue"
        }
    }
//! [define inline component]
//! [create inline component]
    MouseArea {
        anchors.fill: parent
        onClicked: {
            inlinecomponent.createObject(parent)

            var second = inlinecomponent.createObject(parent)

            var third = inlinecomponent.createObject(parent)
            third.x = second.width + 10
            third.color = "red"
        }
    }
//! [create inline component]
//! [parent end]
}
//! [parent end]
//! [document]
