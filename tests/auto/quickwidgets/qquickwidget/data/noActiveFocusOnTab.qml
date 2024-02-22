// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.1

Item {
    Rectangle {
        objectName: "topRect2"
        x: 0
        width: 50
        height: 50
        focus: true
        color: activeFocus ? "green" : "red"
    }
    Rectangle {
        objectName: "middleRect2"
        x: 50
        width: 50
        height: 50
        focus: true
        color: activeFocus ? "green" : "red"
    }
    Rectangle {
        objectName: "bottomRect3"
        x: 100
        width: 50
        height: 50
        focus: true
        color: activeFocus ? "green" : "red"
    }
}

