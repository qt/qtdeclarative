// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [import]
import QtQuick
//! [import]

Row {

//! [simple]
Row {
    Repeater {
        model: 3
        Rectangle {
            width: 100; height: 40
            border.width: 1
            color: "yellow"
        }
    }
}
//! [simple]

//! [index]
Column {
    Repeater {
        model: 10
        Text {
            required property int index
            text: "I'm item " + index
        }
    }
}
//! [index]

//! [modeldata]
Column {
    Repeater {
        model: ["apples", "oranges", "pears"]
        Text {
            required property string modelData
            text: "Data: " + modelData
        }
    }
}
//! [modeldata]

//! [layout]
Row {
    Rectangle { width: 10; height: 20; color: "red" }
    Repeater {
        model: 10
        Rectangle { width: 20; height: 20; radius: 10; color: "green" }
    }
    Rectangle { width: 10; height: 20; color: "blue" }
}
//! [layout]

}
