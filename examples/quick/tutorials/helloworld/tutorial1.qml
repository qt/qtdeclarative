// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
//![3]
import QtQuick
//![3]

//![1]
Rectangle {
    id: page
    width: 320; height: 480
    color: "lightgray"
//![1]

//![2]
    Text {
        id: helloText
        text: "Hello world!"
        y: 30
        anchors.horizontalCenter: page.horizontalCenter
        font.pointSize: 24; font.bold: true
    }
//![2]
}
//![0]
