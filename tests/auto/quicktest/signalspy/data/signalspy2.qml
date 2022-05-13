// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1
import MyImport 1.0

Rectangle {
    id:rect
    width: 200
    height: 200
    color:"red"

    MouseArea {
        id: mouseArea
        anchors.fill: parent
    }

    MyPropertyMap {
        id: propertyMap
        objectName: "propertyMap"
    }

    SignalSpy {
        objectName: "mouseSpy"
        signalName: "pressed"
        target: mouseArea
    }

    SignalSpy {
        objectName: "propertyMapSpy"
        signalName: "mySignal"
        target: propertyMap
    }
}
