// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.2

Item {
    focus: true

    property bool pressed: false
    property bool released: false

    Keys.onPressed: {
        pressed = event.matches(standardKey)
    }

    Keys.onReleased: {
        released = event.matches(standardKey)
    }
}
