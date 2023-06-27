// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "righttoleft"

Column {
    width: 200
//![0]
// automatically aligned to the left
Text {
    text: "Phone"
    width: 200
}

// automatically aligned to the right
Text {
    text: "خامل"
    width: 200
}

// aligned to the left
Text {
    text: "خامل"
    horizontalAlignment: Text.AlignLeft
    width: 200
}

// aligned to the right
Text {
    text: "خامل"
    horizontalAlignment: Text.AlignLeft
    LayoutMirroring.enabled: true
    width: 200
}
//![0]

//![1]
// by default child items are positioned from left to right
Row {
    Child {}
    Child {}
}

// position child items from right to left
Row {
    layoutDirection: Qt.RightToLeft
    Child {}
    Child {}
}

// position child items from left to right
Row {
    LayoutMirroring.enabled: true
    layoutDirection: Qt.RightToLeft
    Child {}
    Child {}
}
//![1]

//![2]
Item {
    height: 50; width: 150

    LayoutMirroring.enabled: true
    anchors.left: parent.left   // anchor left becomes right

    Row {
        // items flow from left to right (as per default)
        Child {}
        Child {}
        Child {}
    }
}
//![2]

//![3]
Item {
    height: 50; width: 150

    LayoutMirroring.enabled: true
    LayoutMirroring.childrenInherit: true
    anchors.left: parent.left   // anchor left becomes right

    Row {
        // setting childrenInherit in the parent causes these
        // items to flow from right to left instead
        Child {}
        Child {}
        Child {}
    }
}
//![3]

//![4]
Rectangle {
    color: "black"
    height: 50; width: 50
    x: mirror(10)
    function mirror(value) {
        return LayoutMirroring.enabled ? (parent.width - width - value) : value;
    }
}
//![4]

//![5]
Image {
    source: "arrow.png"
    mirror: true
}
//![5]
}
