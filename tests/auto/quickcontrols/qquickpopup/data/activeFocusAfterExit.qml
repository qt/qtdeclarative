// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup1: popup1
    property alias popup2: popup2
    property alias popup3: popup3

    Popup {
        id: popup1
        focus: true
        z: 1
    }

    Popup {
        id: popup2
        focus: false
        z: 2
    }

    Popup {
        id: popup3
        focus: true
        z: 3
    }
}
