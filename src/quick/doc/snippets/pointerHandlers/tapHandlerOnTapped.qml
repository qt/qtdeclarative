// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    width: 100
    height: 100

    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onTapped: (eventPoint, button)=> console.log("tapped", eventPoint.device.name,
                                             "button", button,
                                             "@", eventPoint.scenePosition)
    }
}
//![0]
