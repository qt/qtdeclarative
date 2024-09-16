// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias flickable: flickable
    property alias button: button

    Flickable {
        id: flickable
        width: 300
        height: 400
        pressDelay: 50
        Button {
            id: button
            text: "This is a test button"
        }
    }
}
