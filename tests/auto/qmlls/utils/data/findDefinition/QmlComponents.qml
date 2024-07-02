// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import findDefinition.MyApplicationWindowModule

MyApplicationWindow {
    Item {
        property var attachedProperty: MyApplicationWindow.footer
        property var justAnEnum: MyApplicationWindow.Kitty
        property var justAnEnum2: MyApplicationWindow.Hello.Kitty
    }
}
