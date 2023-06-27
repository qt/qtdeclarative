// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }

    width: 640; height: 480
    color: myPalette.window

    Text {
        anchors.fill: parent
        text: "Hello!"; color: myPalette.windowText
    }
}
//![0]
