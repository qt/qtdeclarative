// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Rectangle {
    id: rect
    width: 80; height: 80
    color: "red"

    NumberAnimation on opacity {
        to: 0
        duration: 1000

        onRunningChanged: {
            if (!running) {
                console.log("Destroying...")
                rect.destroy();
            }
        }
    }
}
//![0]
