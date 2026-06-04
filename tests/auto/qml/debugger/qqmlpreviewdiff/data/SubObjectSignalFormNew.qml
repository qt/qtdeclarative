// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as SubObjectSignalFormOld but with a trivial change to the indicator
// color (triggers a rebuild of the form's compilation unit).

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
        color: "blue"
    }
}
