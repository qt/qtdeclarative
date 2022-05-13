// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.12
import QtQuick.Controls 2.12

ApplicationWindow {
    width: 400
    height: 400

    property alias popup1: popup1
    property alias popup2: popup2

    Popup {
        id: popup1
        focus: true
        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 }
        }
    }

    Popup {
        id: popup2
        focus: true
    }
}
