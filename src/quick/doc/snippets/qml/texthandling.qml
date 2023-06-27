// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [document]
import QtQuick


//! [parent begin]
Rectangle {
//! [parent begin]
    width: 300; height: 300
    id: screen

Column {
    anchors.centerIn:parent

//! [int validator]
Column {
    spacing: 10

    Text {
        text: "Enter a value from 0 to 2000"
    }
    TextInput {
        focus: true
        validator: IntValidator { bottom:0; top: 2000}
    }
}
//! [int validator]

//! [regexp validator]
Column {
    spacing: 10

    Text {
        text: "Which basket?"
    }
    TextInput {
        focus: true
        validator: RegularExpressionValidator { regularExpression: /fruit basket/ }
    }
}
//! [regexp validator]

//end of column
}

//! [parent end]
}
//! [parent end]

//! [document]

