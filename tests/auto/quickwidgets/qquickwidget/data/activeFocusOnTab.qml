// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.1

Item {
    Rectangle {
        objectName: "topRect"
        x: 0
        width: 50
        height: 50
        activeFocusOnTab: true
        focus: true
        color: activeFocus ? "green" : "red"
    }
    Rectangle {
        objectName: "middleRect"
        x: 50
        width: 50
        height: 50
        focus: true
        activeFocusOnTab: true
        color: activeFocus ? "green" : "red"
    }
    Rectangle {
        objectName: "bottomRect"
        x: 100
        width: 50
        height: 50
        focus: true
        activeFocusOnTab: true
        color: activeFocus ? "green" : "red"
    }
}

