// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// MyItem.qml
import QtQuick

Item {
    id: item
    width: 100; height: 100

    signal qmlSignal(msg: string)

    MouseArea {
        anchors.fill: parent
        onClicked: item.qmlSignal("Hello from QML")
    }
}
//![0]
