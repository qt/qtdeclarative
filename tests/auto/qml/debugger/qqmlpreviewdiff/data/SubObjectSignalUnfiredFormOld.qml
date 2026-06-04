// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Like SubObjectSignalFormOld but the signal is never fired before rebuild.
// This exercises the case where the endpoint remains in the NotifyList's
// pending 'todo' queue because layout() was never triggered for that index.

import QtQml
import QtQuick

Item {
    id: root
    property alias button: button

    Timer {
        id: button
        interval: 1000
    }

    Rectangle {
        id: indicator
        width: 50
        height: 50
        color: "red"
    }
}
