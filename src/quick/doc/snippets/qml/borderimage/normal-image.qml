// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Rectangle {
    id: page
    color: "white"
    width: 120; height: 120

//! [normal image]
Image {
    source: "pics/borderframe.png"
}
//! [normal image]

    Rectangle {
        x: 30; y: 0
        width: 1; height: 120
        color: "gray"
    }

    Rectangle {
        x: 90; y: 0
        width: 1; height: 120
        color: "gray"
    }

    Rectangle {
        x: 0; y: 30
        width: 200; height: 1
        color: "gray"
    }

    Rectangle {
        x: 0; y: 90
        width: 200; height: 1
        color: "gray"
    }
}
