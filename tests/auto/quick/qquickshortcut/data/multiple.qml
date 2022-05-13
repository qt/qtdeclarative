// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.9
import QtQuick.Window 2.2

Window {
    id: window

    width: 300
    height: 300

    property bool activated: false
    property alias shortcut: shortcut

    Shortcut {
        id: shortcut
        onActivated: window.activated = true
    }
}
