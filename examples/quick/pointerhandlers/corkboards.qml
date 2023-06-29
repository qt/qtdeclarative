// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "components"

Rectangle {
    width: 320; height: 480
    color: "#464646"

    ListModel {
        id: list

        ListElement {
            name: "Panel One"
            notes: [
                ListElement { noteText: "Tap to edit" },
                ListElement { noteText: "Drag to move" },
                ListElement { noteText: "Flick to scroll" }
            ]
        }

        ListElement {
            name: "Panel Two"
            notes: [
                ListElement { noteText: "Note One" },
                ListElement { noteText: "Note Two" }
            ]
        }

        ListElement {
            name: "Panel Three"
            notes: [
                ListElement { noteText: "Note Three" }
            ]
        }
    }

    ListView {
        id: flickable

        anchors.fill: parent
        focus: true
        highlightRangeMode: ListView.StrictlyEnforceRange
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        model: list
        delegate: CorkPanel { objectName: name }
    }
}

