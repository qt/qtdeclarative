// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 400; height: 400

//! [key item]
Item {
    anchors.fill: parent
    focus: true
    Keys.onPressed: (event)=> {
        if (event.key == Qt.Key_Left) {
            console.log("move left");
            event.accepted = true;
        }
    }
}
//! [key item]

Text {
    anchors.fill: parent
    text: "Press a cursor key"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
}
