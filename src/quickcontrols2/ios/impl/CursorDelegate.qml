// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.iOS.impl

Rectangle {
    id: cursor

    color: "#426bf2"
    width: 2
    radius: 1
    visible: parent.activeFocus && !parent.readOnly && parent.selectionStart === parent.selectionEnd

    opacity: timer.visible ? 1 : 0

    CursorFlashTimer {
        id: timer
        cursorPosition: cursor.parent.cursorPosition
        running: cursor.parent.activeFocus && !cursor.parent.readOnly
    }
}
