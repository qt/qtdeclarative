// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick

Item {

    width: 320
    height: 480

    Rectangle {
        color: "#272822"
        width: 320
        height: 480
    }

    //![image]
    // This element displays an image. Because the source is online, it may take some time to fetch
    Image {
        x: 40
        y: 20
        width: 61
        height: 73
        source: "http://codereview.qt-project.org/static/logo_qt.png"
    }
    //![image]
}
//![0]
