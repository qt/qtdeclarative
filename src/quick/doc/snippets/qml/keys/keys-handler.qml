// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 400; height: 400

//! [key item]
Item {
    anchors.fill: parent
    focus: true
    Keys.onLeftPressed: console.log("move left")
}
//! [key item]

Text {
    anchors.fill: parent
    text: "Press a cursor key"
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
}
