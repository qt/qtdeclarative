// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
// Button.qml
import QtQuick

Rectangle {
    id: rect
    width: 100; height: 100

    signal buttonClicked

    MouseArea {
        anchors.fill: parent
        onClicked: rect.buttonClicked()
    }
}
//![0]
