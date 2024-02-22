// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias popup1: popup1
    property alias popup2: popup2

    Button {
        focus: true
    }

    Popup {
        id: popup1
        objectName: "popup1"
        focus: true
        enter: Transition { PauseAnimation { duration: 200 } }
        exit: Transition { PauseAnimation { duration: 200 } }

        Label {
            text: popup1.objectName
            anchors.centerIn: parent
        }
    }

    Popup {
        id: popup2
        objectName: "popup2"
        focus: true
        enter: Transition { PauseAnimation { duration: 100 } }
        exit: Transition { PauseAnimation { duration: 100 } }

        Label {
            text: popup2.objectName
            anchors.centerIn: parent
        }
    }
}
