// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![import]
import QtQuick

//![import]

Column {

//![direct]
Item {
    width: 100; height: 100

    Rectangle {
        // Manually positioned at 20,20
        x: 20
        y: 20
        width: 80
        height: 80
        color: "red"
    }
}
//![direct]

//![anchors]
Item {
    width: 200; height: 200

    Rectangle {
        // Anchored to 20px off the top right corner of the parent
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20 // Sets all margins at once

        width: 80
        height: 80
        color: "orange"
    }

    Rectangle {
        // Anchored to 20px off the top center corner of the parent.
        // Notice the different group property syntax for 'anchors' compared to
        // the previous Rectangle. Both are valid.
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 20 }

        width: 80
        height: 80
        color: "green"
    }
}
//![anchors]

//![positioners]
Item {
    width: 300; height: 100

    Row { // The "Row" type lays out its child items in a horizontal line
        spacing: 20 // Places 20px of space between items

        Rectangle { width: 80; height: 80; color: "red" }
        Rectangle { width: 80; height: 80; color: "green" }
        Rectangle { width: 80; height: 80; color: "blue" }
    }
}
//![positioners]

Item {
    width: 300; height: 400

    Rectangle {
        id: middleRect
        //This Rectangle has its y animated, for the others to follow
        x: 120
        y: 220
        SequentialAnimation on y {
            loops: -1
            NumberAnimation { from: 220; to: 380; easing.type: Easing.InOutQuad; duration: 1200 }
            NumberAnimation { from: 380; to: 220; easing.type: Easing.InOutQuad; duration: 1200 }
        }
        width: 80
        height: 80
        color: "green"
    }
    Rectangle {
        // x,y bound to the position of middleRect
        x: middleRect.x - 100
        y: middleRect.y
        width: 80
        height: 80
        color: "red"
    }

    Rectangle {
        // Anchored to the position of middleRect
        anchors.left: middleRect.right
        anchors.leftMargin: 20
        anchors.verticalCenter: middleRect.verticalCenter
        width: 80
        height: 80
        color: "blue"
    }
}

}
