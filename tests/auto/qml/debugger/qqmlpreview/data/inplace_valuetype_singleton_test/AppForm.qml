// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: root
    width: 200
    height: 100
    property alias themeButton: themeButton

    Button {
        id: themeButton
        // A value-type group-property binding (icon.color) that reads the Colors
        // singleton. This is the binding that must be torn down when the button is
        // recreated on reload.
        icon.color: (Colors.currentTheme == Colors.dark) ? "#ffffff" : "#000000"
    }
}
