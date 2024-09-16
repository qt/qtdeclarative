// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import findDefinition.MyApplicationWindowModule as MAWM

MAWM.MyApplicationWindow {
    Item {
        property var attachedProperty: MAWM.MyApplicationWindow.footer
        property var justAnEnum: MAWM.MyApplicationWindow.Kitty
        property var justAnEnum2: MAWM.MyApplicationWindow.Hello.Kitty
    }
}
